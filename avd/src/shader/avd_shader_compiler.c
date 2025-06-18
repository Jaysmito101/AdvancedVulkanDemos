#include "shader/avd_shader_compiler.h"
#include "core/avd_core.h"
#include "avd_asset.h"

#include "shaderc/shaderc.h"

static shaderc_include_result* __avdIncludeResolver(void *userData, const char *requestedSource, int type, const char *requestingSource, size_t includeDepth)
{
    AVD_ASSERT(requestedSource != NULL);

    shaderc_include_result* result = (shaderc_include_result*)malloc(sizeof(*result));
    if (!result) {
        AVD_LOG("Failed to allocate memory for include result\n");
        return NULL;
    }

    result->source_name = requestedSource;
    result->content = avdAssetShader(requestedSource);
    result->content_length = strlen(result->content);
    result->source_name = requestedSource;
    result->source_name_length = strlen(requestedSource);
    result->user_data = NULL;
    return result;
}

static void __avdIncludeResultReleaser(void *userData, shaderc_include_result *includeResult)
{
    (void)userData; (void)includeResult;
}

uint32_t *avdCompileShader(const char *shaderCode, const char *inputFileName, size_t *outSize)
{
    shaderc_compiler_t compiler       = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_source_language(options, shaderc_source_language_glsl);
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
    shaderc_compile_options_set_warnings_as_errors(options);
    shaderc_compile_options_set_include_callbacks(options, __avdIncludeResolver, __avdIncludeResultReleaser, NULL);

#ifdef AVD_DEBUG
    AVD_LOG("Compiling shader: %s with debug info enabled\n", inputFileName);
    shaderc_compile_options_set_generate_debug_info(options);
#endif

    shaderc_shader_kind kind = shaderc_glsl_infer_from_source;

    shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, shaderCode, strlen(shaderCode), kind, inputFileName, "main", options);
    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        avdPrintShaderWithLineNumbers(shaderCode, inputFileName);
        AVD_LOG("Shader compilation failed: %s\n", shaderc_result_get_error_message(result));
        return NULL;
    }

    size_t size = shaderc_result_get_length(result);
    if (size % 4 != 0) {
        AVD_LOG("Shader compilation result size is not a multiple of 4\n");
        shaderc_result_release(result);
        shaderc_compile_options_release(options);
        shaderc_compiler_release(compiler);
        return NULL;
    }

    uint32_t *compiledCode = (uint32_t *)malloc(size);
    if (!compiledCode) {
        AVD_LOG("Failed to allocate memory for compiled shader\n");
        shaderc_result_release(result);
        shaderc_compile_options_release(options);
        shaderc_compiler_release(compiler);
        return NULL;
    }

    memcpy(compiledCode, shaderc_result_get_bytes(result), size);
    *outSize = size / 4;

    shaderc_result_release(result);
    shaderc_compile_options_release(options);
    shaderc_compiler_release(compiler);

    return compiledCode;
}

uint32_t *avdCompileShaderAndCache(const char *shaderCode, const char *inputFileName, size_t *outSize)
{
    static char cacheFilePath[1024 * 4];
    uint32_t shaderHash = avdHashString(shaderCode);
#ifdef AVD_DEBUG
    snprintf(cacheFilePath, sizeof(cacheFilePath), "%s%u.%s.debug.psshader", avdGetTempDirPath(), shaderHash, inputFileName);
#else
    snprintf(cacheFilePath, sizeof(cacheFilePath), "%s%u.%s.psshader", avdGetTempDirPath(), shaderHash, inputFileName);
#endif

    FILE *file = fopen(cacheFilePath, "rb");
    if (file) {
        AVD_LOG("Loading shader from cache: %s\n", cacheFilePath);
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file) / sizeof(uint32_t);
        fseek(file, 0, SEEK_SET);
        uint32_t *compiledCode = (uint32_t *)malloc(size * sizeof(uint32_t));
        fread(compiledCode, sizeof(uint32_t), size, file);
        fclose(file);
        *outSize = size;
        return compiledCode;
    } else {
        AVD_LOG("Cache file not found, compiling shader...\n");
        uint32_t *compiledCode = avdCompileShader(shaderCode, inputFileName, outSize);
        if (compiledCode) {
            AVD_LOG("Saving shader to cache: %s\n", cacheFilePath);
            file = fopen(cacheFilePath, "wb");
            if (file) {
                fwrite(compiledCode, sizeof(uint32_t), *outSize, file);
                fclose(file);
            }
        }
        return compiledCode;
    }
}

void avdFreeShader(uint32_t *shaderCode)
{
    if (shaderCode) {
        free(shaderCode);
    }
}