#include "avd_shader.h"
#include "avd_asset.h"

VkShaderModule avdShaderModuleCreate(VkDevice device, const char *shaderCode, VkShaderStageFlagBits shaderType, const char *inputFileName)
{
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(shaderCode != NULL);
    AVD_ASSERT(inputFileName != NULL);

    size_t shaderSize = 0;
    uint32_t *compiledShader = avdCompileShaderAndCache(shaderCode, inputFileName, &shaderSize);
    if (compiledShader == NULL)
    {
        AVD_LOG("Failed to compile shader: %s\n", inputFileName);
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderSize * sizeof(uint32_t);
    createInfo.pCode = compiledShader;

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, NULL, &shaderModule);
    if (result != VK_SUCCESS)
    {
        AVD_LOG("Failed to create shader module: %s\n", inputFileName);
        free(compiledShader);
        return VK_NULL_HANDLE;
    }

    free(compiledShader);
    return shaderModule;
}

VkShaderModule avdShaderModuleCreateFromAsset(VkDevice device, const char* asset) {
    VkShaderStageFlagBits shaderType = (VkShaderStageFlagBits)0;
    if (strstr(asset, "Vert") != NULL) {
        shaderType = VK_SHADER_STAGE_VERTEX_BIT;
    } else if (strstr(asset, "Frag") != NULL) {
        shaderType = VK_SHADER_STAGE_FRAGMENT_BIT;
    } else if (strstr(asset, "Comp") != NULL) {
        shaderType = VK_SHADER_STAGE_COMPUTE_BIT;
    } else {
        AVD_LOG("Unknown shader type for asset: %s\n", asset);
        return VK_NULL_HANDLE;
    }

    VkShaderModule shaderModule = avdShaderModuleCreate(device, avdAssetShader(asset), shaderType, asset);
    AVD_CHECK_VK_HANDLE(shaderModule, "Failed to create shader module from asset: %s\n", asset);
    return shaderModule;
}