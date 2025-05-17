#ifndef AVD_SHADERS_H
#define AVD_SHADERS_H

#include "core/avd_core.h"
#include "vulkan/avd_vulkan.h"
#include "shader/avd_shader_compiler.h"


VkShaderModule avdShaderModuleCreate(VkDevice device, const char* shaderCode, VkShaderStageFlagBits shaderType, const char* inputFileName);
VkShaderModule avdShaderModuleCreateFromAsset(VkDevice device, const char* asset);


#endif // AVD_SHADERS_H
