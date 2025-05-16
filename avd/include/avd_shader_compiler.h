#ifndef AVD_SHADER_COMPILER_H
#define AVD_SHADER_COMPILER_H

#include "avd_common.h"
#include "vulkan/vulkan.h"


uint32_t* avdCompileShader(const char* shaderCode, const char* inputFileName, size_t* outSize);
uint32_t* avdCompileShaderAndCache(const char* shaderCode, const char* inputFileName, size_t* outSize);
void avdFreeShader(uint32_t* shaderCode);


#endif // AVD_SHADER_COMPILER_H