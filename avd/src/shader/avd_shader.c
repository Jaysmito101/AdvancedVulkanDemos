#include "shader/avd_shader.h"
#include "avd_asset.h"
#include "shader/avd_shader_shaderc.h"
#include "shader/avd_shader_slang.h"

// Since this would be needed in a lot of places in code, its much
// easier to have it be like a global singleton here,
// so that we can access it from anywhere in the codebase.
static AVD_ShaderManager* __AVD_SHADER_MANAGER_CACHE = NULL;

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
    switch (avdAssetShaderLanguage(inputShaderName)) 
    {
        case AVD_SHADER_LANGUAGE_GLSL:
        case AVD_SHADER_LANGUAGE_HLSL:
            AVD_CHECK(avdShaderShaderCCompile(manager->shaderCContext, inputShaderName, options, outResult));
            break;
        case AVD_SHADER_LANGUAGE_SLANG:
            AVD_CHECK(avdShaderSlangCompile(manager->slangContext, inputShaderName, options, outResult));
            break;
        default:
            AVD_CHECK_MSG(false, "Unsupported shader language for %s, are you sure this is a valid shader asset?", inputShaderName);
    };
    return true;
}

bool avdShaderManagerCompileAndCache(
    AVD_ShaderManager *manager,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult)
{
    return avdShaderManagerCompile(
        manager,
        inputShaderName,
        options,
        outResult);
}
