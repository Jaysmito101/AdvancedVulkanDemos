#ifndef AVD_SHADER_SHADERC_H
#define AVD_SHADER_SHADERC_H

#include "shader/avd_shader_base.h"
#include "shaderc/shaderc.h"

struct AVD_ShaderShaderCContext {
    void* userData;
};

bool avdShaderShaderCContextInit(AVD_ShaderShaderCContext *context);
void avdShaderShaderCContextDestroy(AVD_ShaderShaderCContext *context);
bool avdShaderShaderCCompile(
    AVD_ShaderShaderCContext *context,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult
);


#endif // AVD_SHADER_SHADERC_H
