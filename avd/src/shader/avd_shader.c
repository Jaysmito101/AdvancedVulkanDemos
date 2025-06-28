#include "shader/avd_shader.h"
#include "avd_asset.h"
#include "shader/avd_shader_shaderc.h"
#include "shader/avd_shader_slang.h"

// Since this would be needed in a lot of places in code, its much
// easier to have it be like a global singleton here,
// so that we can access it from anywhere in the codebase.
static AVD_ShaderManager *__AVD_SHADER_MANAGER_CACHE = NULL;

bool avdShaderManagerInit(AVD_ShaderManager *shaderManager)
{
    shaderManager->shaderCContext = (AVD_ShaderShaderCContext *)malloc(sizeof(AVD_ShaderShaderCContext));
    AVD_CHECK_MSG(shaderManager->shaderCContext != NULL, "Failed to allocate shaderCContext");
    AVD_CHECK(avdShaderShaderCContextInit(shaderManager->shaderCContext));

    shaderManager->slangContext = (AVD_ShaderSlangContext *)malloc(sizeof(AVD_ShaderSlangContext));
    AVD_CHECK_MSG(shaderManager->slangContext != NULL, "Failed to allocate slangContext");
    AVD_CHECK(avdShaderSlangContextInit(shaderManager->slangContext));

    __AVD_SHADER_MANAGER_CACHE = shaderManager;

    return true;
}

void avdShaderManagerDestroy(AVD_ShaderManager *shaderManager)
{
    if (shaderManager->shaderCContext != NULL) {
        avdShaderShaderCContextDestroy(shaderManager->shaderCContext);
        free(shaderManager->shaderCContext);
    }

    if (shaderManager->slangContext != NULL) {
        avdShaderSlangContextDestroy(shaderManager->slangContext);
        free(shaderManager->slangContext);
    }

    __AVD_SHADER_MANAGER_CACHE = NULL;
}

bool avdShaderModuleCreateWithoutCache(
    VkDevice device,
    const char *shaderName,
    AVD_ShaderCompilationOptions *options, // optional
    VkShaderModule *outModule)
{
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(shaderName != NULL);

    AVD_ShaderCompilationResult compilationResult      = {0};
    static AVD_ShaderCompilationOptions defaultOptions = {0};
    if (options == NULL) {
        avdShaderCompilationOptionsDefault(&defaultOptions);
        options = &defaultOptions;
    }
    AVD_CHECK(avdShaderManagerCompile(__AVD_SHADER_MANAGER_CACHE, shaderName, options, &compilationResult));

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize                 = compilationResult.size * sizeof(uint32_t);
    createInfo.pCode                    = compilationResult.compiledCode;

    AVD_CHECK_VK_RESULT(vkCreateShaderModule(device, &createInfo, NULL, outModule),
                        "Failed to create shader module from asset: %s", shaderName);
    return true;
}

bool avdShaderModuleCreate(
    VkDevice device,
    const char *shaderName,
    AVD_ShaderCompilationOptions *options, // optional
    VkShaderModule *outModule)
{
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(shaderName != NULL);

    AVD_ShaderCompilationResult compilationResult      = {0};
    static AVD_ShaderCompilationOptions defaultOptions = {0};
    if (options == NULL) {
        avdShaderCompilationOptionsDefault(&defaultOptions);
        options = &defaultOptions;
    }
    AVD_CHECK(avdShaderManagerCompileAndCache(__AVD_SHADER_MANAGER_CACHE, shaderName, options, &compilationResult));

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize                 = compilationResult.size * sizeof(uint32_t);
    createInfo.pCode                    = compilationResult.compiledCode;

    AVD_CHECK_VK_RESULT(vkCreateShaderModule(device, &createInfo, NULL, outModule),
                        "Failed to create shader module from asset: %s", shaderName);
    return true;
}

bool avdShaderManagerCompile(
    AVD_ShaderManager *manager,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult)
{
    AVD_ASSERT(manager != NULL);
    AVD_ASSERT(inputShaderName != NULL);
    AVD_ASSERT(outResult != NULL);

    switch (avdAssetShaderLanguage(inputShaderName)) {
        case AVD_SHADER_LANGUAGE_GLSL:
        case AVD_SHADER_LANGUAGE_HLSL:
            AVD_CHECK(avdShaderShaderCCompile(manager->shaderCContext, inputShaderName, options, outResult));
            break;
        case AVD_SHADER_LANGUAGE_SLANG:
            AVD_CHECK(avdShaderSlangCompile(manager->slangContext, inputShaderName, options, outResult));
            break;
        default:
            AVD_CHECK_MSG(false, "Unsupported shader language for %s, are you sure this is a valid shader asset?\n", inputShaderName);
    };
    return true;
}

bool __avdShaderLoadCached(
    const char *cachedShaderPath,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult)
{
    AVD_ASSERT(cachedShaderPath != NULL);
    AVD_ASSERT(inputShaderName != NULL);
    AVD_ASSERT(outResult != NULL);

    AVD_LOG("Using cached shader: %s\n", cachedShaderPath);
    FILE *file = fopen(cachedShaderPath, "rb");
    AVD_CHECK_MSG(file != NULL, "Failed to open cached shader file: %s", cachedShaderPath);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint32_t *compiledCode = (uint32_t *)malloc(size);
    AVD_CHECK_MSG(compiledCode != NULL, "Failed to allocate memory for compiled shader");

    fread(compiledCode, 1, size, file);
    fclose(file);

    *outResult = (AVD_ShaderCompilationResult){
        .compiledCode = compiledCode,
        .size         = size / sizeof(uint32_t),
        .stage        = avdAssetShaderStage(inputShaderName),
        .language     = avdAssetShaderLanguage(inputShaderName)};

    return true;
}

bool __avdShaderSaveCached(
    const char *cachedShaderPath,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult)
{
    AVD_ASSERT(cachedShaderPath != NULL);
    AVD_ASSERT(inputShaderName != NULL);
    AVD_ASSERT(outResult != NULL);

    FILE *file = fopen(cachedShaderPath, "wb");
    if (file == NULL) {
        AVD_LOG("Failed to open cached shader file for writing: %s\n", cachedShaderPath);
        return false;
    }

    fwrite(outResult->compiledCode, sizeof(uint32_t), outResult->size, file);
    fclose(file);

    AVD_LOG("Cached shader saved: %s\n", cachedShaderPath);
    return true;
}

bool avdShaderManagerCompileAndCache(
    AVD_ShaderManager *manager,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult)
{
    AVD_ASSERT(manager != NULL);
    AVD_ASSERT(inputShaderName != NULL);
    AVD_ASSERT(outResult != NULL);

    uint32_t shaderDependencyHash = avdAssetShaderGetDependencyHash(inputShaderName);
    uint32_t optionsHash          = avdShaderCompilationOptionsHash(options);
    uint32_t cacheKey             = shaderDependencyHash ^ optionsHash;

    static char cachedShaderPath[1024 * 4] = {0};
    snprintf(
        cachedShaderPath,
        sizeof(cachedShaderPath),
        "%savd_%s.%s.%s.%u.avdshader",
        avdGetTempDirPath(),
        inputShaderName,
        avdShaderStageToString(avdAssetShaderStage(inputShaderName)),
        avdShaderLanguageToString(avdAssetShaderLanguage(inputShaderName)),
        cacheKey);

    if (avdPathExists(cachedShaderPath)) {
        AVD_CHECK(__avdShaderLoadCached(
            cachedShaderPath,
            inputShaderName,
            options,
            outResult));
    } else {
        AVD_CHECK(avdShaderManagerCompile(
            manager,
            inputShaderName,
            options,
            outResult));

        AVD_CHECK(__avdShaderSaveCached(
            cachedShaderPath,
            inputShaderName,
            options,
            outResult));
    }

    return true;
}
