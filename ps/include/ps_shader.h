#ifndef PS_SHADERS_H
#define PS_SHADERS_H

#include "ps_common.h"
#include "ps_shader_compiler.h"
#include "ps_vulkan.h"


VkShaderModule psShaderModuleCreate(VkDevice device, const char* shaderCode, VkShaderStageFlagBits shaderType, const char* inputFileName);
VkShaderModule psShaderModuleCreateFromAsset(VkDevice device, const char* asset);


#endif // PS_SHADERS_H
