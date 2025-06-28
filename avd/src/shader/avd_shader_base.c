#include "shader/avd_shader_base.h"

bool avdShaderCompilationOptionsDefault(AVD_ShaderCompilationOptions* options)
{
    AVD_ASSERT(options != NULL);

    options->macros = NULL;
    options->macroCount = 0;
    options->warningsAsErrors = true;
    options->optimize = 2; // Default to performance optimization
    
    #ifdef AVD_DEBUG
    options->debugSymbols = true;
#else 
    options->debugSymbols = false;
#endif

    return true;
}

uint32_t avdShaderCompilationOptionsHash(const AVD_ShaderCompilationOptions* options)
{
    AVD_ASSERT(options != NULL);

    uint32_t hash = 5381; // Starting with a prime number
    for (size_t i = 0; i < options->macroCount; ++i) {
        const char* macro = options->macros[i];
        while (*macro) {
            hash = ((hash << 5) + hash) + *macro++; // hash * 33 + c
        }
    }
    hash ^= options->warningsAsErrors ? 0x1 : 0x0;
    hash ^= options->debugSymbols ? 0x2 : 0x0;
    hash ^= options->optimize;

    return hash;
}

const char* avdShaderStageToString(AVD_ShaderStage stage)
{
    switch (stage) {
        case AVD_SHADER_STAGE_VERTEX:
            return "Vertex";
        case AVD_SHADER_STAGE_FRAGMENT:
            return "Fragment";
        case AVD_SHADER_STAGE_COMPUTE:
            return "Compute";
        case AVD_SHADER_STAGE_HEADER:
            return "Header";
        default:
            return "Unknown";
    }
}

const char* avdShaderLanguageToString(AVD_ShaderLanguage language)
{
    switch (language) {
        case AVD_SHADER_LANGUAGE_GLSL:
            return "GLSL";
        case AVD_SHADER_LANGUAGE_SLANG:
            return "SLANG";
        default:
            return "Unknown";
    }
}

void avdShaderCompilationResultDestroy(AVD_ShaderCompilationResult* result)
{
    if (result) {
        free(result->compiledCode);
    }
}