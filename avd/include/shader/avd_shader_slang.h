#ifndef AVD_SHADER_SLANG_H
#define AVD_SHADER_SLANG_H

#include "shader/avd_shader_base.h"
#include "shaderc/shaderc.h"

struct AVD_ShaderSlangContext {
    void* userData;
};

bool avdShaderSlangContextInit(AVD_ShaderSlangContext *context);
void avdShaderSlangContextDestroy(AVD_ShaderSlangContext *context);
bool avdShaderSlangCompile(
    AVD_ShaderSlangContext *context,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult
);


#endif // AVD_SHADER_SLANG_H
