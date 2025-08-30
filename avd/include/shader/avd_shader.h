#ifndef AVD_SHADERS_H
#define AVD_SHADERS_H

#include "math/avd_math.h"
#include "shader/avd_shader_base.h"
#include "vulkan/avd_vulkan_base.h"

bool avdShaderManagerInit(AVD_ShaderManager *shaderManager);
void avdShaderManagerDestroy(AVD_ShaderManager *shaderManager);

bool avdShaderManagerCompile(
    AVD_ShaderManager *manager,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult);
bool avdShaderManagerCompileAndCache(
    AVD_ShaderManager *manager,
    const char *inputShaderName,
    AVD_ShaderCompilationOptions *options,
    AVD_ShaderCompilationResult *outResult);

bool avdShaderModuleCreateWithoutCache(
    VkDevice device,
    const char *shaderName,
    AVD_ShaderCompilationOptions *options, // optional
    VkShaderModule *outModule);
bool avdShaderModuleCreate(
    VkDevice device,
    const char *shaderName,
    AVD_ShaderCompilationOptions *options, // optional
    VkShaderModule *outModule);

#endif // AVD_SHADERS_H
