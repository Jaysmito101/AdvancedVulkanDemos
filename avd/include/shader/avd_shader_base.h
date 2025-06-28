#ifndef AVD_SHADER_BASE_H
#define AVD_SHADER_BASE_H

#include "core/avd_core.h"

typedef enum {
    AVD_SHADER_STAGE_VERTEX = 0,
    AVD_SHADER_STAGE_FRAGMENT,
    AVD_SHADER_STAGE_COMPUTE,
    AVD_SHADER_STAGE_HEADER
} AVD_ShaderStage;

typedef enum {
    AVD_SHADER_LANGUAGE_GLSL = 0,
    AVD_SHADER_LANGUAGE_HLSL,
    AVD_SHADER_LANGUAGE_SLANG,
    AVD_SHADER_LANGUAGE_UNKNOWN
} AVD_ShaderLanguage;

typedef struct {
    const char** macros;
    size_t macroCount;
    bool warningsAsErrors; 
    bool debugSymbols;
    uint8_t optimize; // 0 = no optimization, 1 = size optimization, 2 = performance optimization
} AVD_ShaderCompilationOptions;

typedef struct {
    uint32_t* compiledCode;
    size_t size;
    AVD_ShaderStage stage;
    AVD_ShaderLanguage language;
} AVD_ShaderCompilationResult;

typedef struct AVD_ShaderShaderCContext AVD_ShaderShaderCContext;
typedef struct AVD_ShaderSlangContext AVD_ShaderSlangContext;

typedef struct {
    AVD_ShaderShaderCContext *shaderCContext;
    AVD_ShaderSlangContext *slangContext;
} AVD_ShaderManager;


bool avdShaderCompilationOptionsDefault(AVD_ShaderCompilationOptions* options);
uint32_t avdShaderCompilationOptionsHash(const AVD_ShaderCompilationOptions* options);
const char* avdShaderStageToString(AVD_ShaderStage stage);
const char* avdShaderLanguageToString(AVD_ShaderLanguage language);
void avdShaderCompilationResultDestroy(AVD_ShaderCompilationResult* result);

#endif