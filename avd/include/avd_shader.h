#ifndef AVD_SHADERS_H
#define AVD_SHADERS_H

#include "avd_common.h"
#include "avd_shader_compiler.h"
#include "avd_vulkan.h"


VkShaderModule avdShaderModuleCreate(VkDevice device, const char* shaderCode, VkShaderStageFlagBits shaderType, const char* inputFileName);
VkShaderModule avdShaderModuleCreateFromAsset(VkDevice device, const char* asset);


#endif // AVD_SHADERS_H
