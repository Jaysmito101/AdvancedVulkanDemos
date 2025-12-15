#include "shader/avd_shader_slang.h"
#include "avd_asset.h"

bool avdShaderSlangContextInit(AVD_ShaderSlangContext *context)
{
    AVD_LOG_WARN("----------------------IMPORTANT------------------------------------");
    AVD_LOG_WARN("Slang currently doesnt have a working C API.");
    AVD_LOG_WARN("Refer to: https://github.com/shader-slang/slang/issues/5565");
    AVD_LOG_WARN("And I am very reluctant to use C++ in this project.");
    AVD_LOG_WARN("Thus slang compilation path will remain unimplemented for now.");
    AVD_LOG_WARN("-------------------------------------------------------------------");
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
