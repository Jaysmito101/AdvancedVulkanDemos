#ifndef PS_SHADERS_H
#define PS_SHADERS_H

#include "ps_common.h"
#include "ps_shader_compiler.h"
#include "ps_vulkan.h"


VkShaderModule psShaderModuleCreate(PS_GameState *gameState, const char* shaderCode, VkShaderStageFlagBits shaderType, const char* inputFileName);


#endif // PS_SHADERS_H
