#include "shader/avd_shader_slang.h"
#include "avd_asset.h"

bool avdShaderSlangContextInit(AVD_ShaderSlangContext *context)
{
    context->userData = NULL;
    return true;
}

void avdShaderSlangContextDestroy(AVD_ShaderSlangContext *context)
{
}

bool avdShaderSlangCompile(
    AVD_ShaderSlangContext *context,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult)
{
    AVD_CHECK_MSG(false, "SLANG shader compilation is not implemented");
    return true;
}
