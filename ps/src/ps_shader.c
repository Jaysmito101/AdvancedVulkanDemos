#include "ps_shader.h"

VkShaderModule psShaderModuleCreate(PS_GameState *gameState, const char *shaderCode, VkShaderStageFlagBits shaderType, const char *inputFileName)
{
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(shaderCode != NULL);
    PS_ASSERT(inputFileName != NULL);

    size_t shaderSize = 0;
    uint32_t *compiledShader = psCompileShaderAndCache(shaderCode, inputFileName, &shaderSize);
    if (compiledShader == NULL)
    {
        PS_LOG("Failed to compile shader: %s\n", inputFileName);
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderSize * sizeof(uint32_t);
    createInfo.pCode = compiledShader;

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(gameState->vulkan.device, &createInfo, NULL, &shaderModule);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create shader module: %s\n", inputFileName);
        free(compiledShader);
        return VK_NULL_HANDLE;
    }

    free(compiledShader);
    return shaderModule;
}