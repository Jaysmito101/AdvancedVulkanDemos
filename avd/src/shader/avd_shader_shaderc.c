#include "shader/avd_shader_shaderc.h"
#include "avd_asset.h"

static shaderc_include_result *__avdIncludeResolver(void *userData, const char *requestedSource, int type, const char *requestingSource, size_t includeDepth)
{
    AVD_ASSERT(requestedSource != NULL);

    shaderc_include_result *result = (shaderc_include_result *)malloc(sizeof(*result));
    if (!result) {
        AVD_LOG("Failed to allocate memory for include result\n");
        return NULL;
    }

    result->source_name        = requestedSource;
    result->content            = avdAssetShader(requestedSource);
    result->content_length     = strlen(result->content);
    result->source_name        = requestedSource;
    result->source_name_length = strlen(requestedSource);
    result->user_data          = NULL;
    return result;
}

static void __avdIncludeResultReleaser(void *userData, shaderc_include_result *includeResult)
{
    (void)userData;
    if (includeResult) {
        free(includeResult);
    }
}

bool avdShaderShaderCContextInit(AVD_ShaderShaderCContext *context)
{
    context->userData = NULL;
    return true;
}

void avdShaderShaderCContextDestroy(AVD_ShaderShaderCContext *context)
{
}

bool avdShaderShaderCCompile(
    AVD_ShaderShaderCContext *context,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *inOptions,
    AVD_ShaderCompilationResult *outResult)
{
    shaderc_compiler_t compiler       = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    switch (avdAssetShaderLanguage(inputShaderName)) {
        case AVD_SHADER_LANGUAGE_GLSL:
            shaderc_compile_options_set_source_language(options, shaderc_source_language_glsl);
            break;
        case AVD_SHADER_LANGUAGE_HLSL:
            shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
            break;
        default:
            AVD_CHECK_MSG(false, "Unsupported shader language for %s, are you sure you are using the correct compiler context?", inputShaderName);
    }
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_zero + inOptions->optimize);
    if (inOptions->warningsAsErrors) {
        AVD_LOG("Compiling shader: %s with warnings as errors enabled\n", inputShaderName);
        shaderc_compile_options_set_warnings_as_errors(options);
    }
    shaderc_compile_options_set_include_callbacks(options, __avdIncludeResolver, __avdIncludeResultReleaser, NULL);
    if (inOptions->macros && inOptions->macroCount > 0) {
        for (size_t i = 0; i < inOptions->macroCount; ++i) {
            const char *macro = inOptions->macros[i];
            const char *equalSign = strchr(macro, '=');
            if (equalSign) {
                size_t nameLength = equalSign - macro;
                shaderc_compile_options_add_macro_definition(options, macro, nameLength, equalSign + 1, strlen(equalSign + 1));
            } else {
                shaderc_compile_options_add_macro_definition(options, macro, strlen(macro), NULL, 0);
            }
        }
    }

    if (inOptions->debugSymbols) {
        AVD_LOG("Compiling shader: %s with debug info enabled\n", inputShaderName);
        shaderc_compile_options_set_generate_debug_info(options);
    }

    shaderc_shader_kind kind = shaderc_glsl_infer_from_source;

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        compiler,
        avdAssetShader(inputShaderName),
        strlen(avdAssetShader(inputShaderName)),
        kind,
        inputShaderName,
        "main",
        options);
    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        avdPrintShaderWithLineNumbers(avdAssetShader(inputShaderName), inputShaderName);
        AVD_LOG("Shader compilation failed: %s\n", shaderc_result_get_error_message(result));
        return false;
    }

    size_t size = shaderc_result_get_length(result);
    if (size % 4 != 0) {
        AVD_LOG("Shader compilation result size is not a multiple of 4\n");
        shaderc_result_release(result);
        shaderc_compile_options_release(options);
        shaderc_compiler_release(compiler);
        return false;
    }

    uint32_t *compiledCode = (uint32_t *)malloc(size);
    if (!compiledCode) {
        AVD_LOG("Failed to allocate memory for compiled shader\n");
        shaderc_result_release(result);
        shaderc_compile_options_release(options);
        shaderc_compiler_release(compiler);
        return false;
    }

    memcpy(compiledCode, shaderc_result_get_bytes(result), size);

    shaderc_result_release(result);
    shaderc_compile_options_release(options);
    shaderc_compiler_release(compiler);

    *outResult = (AVD_ShaderCompilationResult){
        .compiledCode = compiledCode,
        .size         = size / sizeof(uint32_t),
        .stage        = avdAssetShaderStage(inputShaderName),
        .language     = avdAssetShaderLanguage(inputShaderName)
    };

    return true;
}
