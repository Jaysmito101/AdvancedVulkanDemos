#include "bloom/avd_bloom.h"

typedef struct AVD_BloomUberPushConstants {
    float bloomWidth;
    float bloomHeight;
} AVD_BloomUberPushConstants;

static bool __avdBloomCreateFramebuffers(AVD_Bloom *bloom, AVD_Vulkan *vulkan, uint32_t width, uint32_t height)
{
    AVD_ASSERT(bloom != NULL);
    AVD_ASSERT(vulkan != NULL);

    for (uint32_t i = 0; i < bloom->passCount; ++i) {
        uint32_t framebufferWidth          = width >> i;
        uint32_t framebufferHeight         = height >> i;
        AVD_VulkanFramebuffer *framebuffer = &bloom->bloomPasses[i];
        AVD_CHECK(avdVulkanFramebufferCreate(
            vulkan,
            framebuffer,
            framebufferWidth,
            framebufferHeight,
            false,
            VK_FORMAT_R32G32B32A32_SFLOAT, // could play with this a bit for a more efficient format
            VK_FORMAT_D32_SFLOAT_S8_UINT));
    }

    return true;
}

bool avdBloomCreate(AVD_Bloom *bloom, AVD_Vulkan *vulkan, uint32_t width, uint32_t height)
{
    // build the framebuffers
    bloom->width     = width;
    bloom->height    = height;
    bloom->passCount = 5;

    AVD_CHECK(__avdBloomCreateFramebuffers(bloom, vulkan, width, height));

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &bloom->bloomDescriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));
    AVD_CHECK(avdPipelineUtilsCreateGraphicsPipelineLayout(
        &bloom->pipelineLayout,
        vulkan->device,
        &bloom->bloomDescriptorSetLayout, 1,
        sizeof(AVD_BloomUberPushConstants)));
    AVD_CHECK(avdPipelineUtilsCreateGenericGraphicsPipeline(
        &bloom->pipeline,
        bloom->pipelineLayout,
        vulkan->device,
        bloom->bloomPasses[0].renderPass,
        "PresentationVert",
        "PresentationFrag"));

    return true;
}

void avdBloomDestroy(AVD_Bloom *bloom, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(bloom != NULL);
    AVD_ASSERT(vulkan != NULL);

    for (uint32_t i = 0; i < bloom->passCount; ++i) {
        avdVulkanFramebufferDestroy(vulkan, &bloom->bloomPasses[i]);
    }
    vkDestroyPipeline(vulkan->device, bloom->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, bloom->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, bloom->bloomDescriptorSetLayout, NULL);
}
