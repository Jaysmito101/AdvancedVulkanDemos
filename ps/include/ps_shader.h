#ifndef PS_SHADERS_H
#define PS_SHADERS_H

#include "ps_common.h"
#include "ps_shader_compiler.h"
#include "ps_vulkan.h"

extern const char* psShader_PresentationVertex;
extern const char* psShader_PresentationFragment;

extern const char* psShader_SplashSceneVertex;
extern const char* psShader_SplashSceneFragment;

extern const char* psShader_LoadingVertex;
extern const char* psShader_LoadingFragment;
 

VkShaderModule psShaderModuleCreate(PS_GameState *gameState, const char* shaderCode, VkShaderStageFlagBits shaderType, const char* inputFileName);

#endif // PS_SHADERS_H
