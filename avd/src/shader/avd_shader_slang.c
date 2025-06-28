#include "shader/avd_shader_slang.h"
#include "avd_asset.h"

bool avdShaderSlangContextInit(AVD_ShaderSlangContext *context)
{
    AVD_LOG("----------------------IMPORTANT------------------------------------\n");
    AVD_LOG("Slang currently doesnt have a working C API.\n");
    AVD_LOG("Refer to: https://github.com/shader-slang/slang/issues/5565\n");
    AVD_LOG("And I am very reluctant to use C++ in this project.\n");
    AVD_LOG("Thus slang compilation path will remain unimplemented for now.\n");
    AVD_LOG("-------------------------------------------------------------------\n");
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
