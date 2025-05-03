#ifndef PS_SHADER_COMPILER_H
#define PS_SHADER_COMPILER_H

#include "ps_common.h"
#include "vulkan/vulkan.h"


uint32_t* psCompileShader(const char* shaderCode, const char* inputFileName, size_t* outSize);
uint32_t* psCompileShaderAndCache(const char* shaderCode, const char* inputFileName, size_t* outSize);
void psFreeShader(uint32_t* shaderCode);


#endif // PS_SHADER_COMPILER_H