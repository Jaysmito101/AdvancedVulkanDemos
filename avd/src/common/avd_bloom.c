#include "common/avd_bloom.h"

typedef struct AVD_BloomUberPushConstants {
    float srcWidth;
    float srcHeight;
    float targetWidth;
    float targetHeight;

    int bloomPrefilterType;
    int type;
    float bloomPrefilterThreshold;
    float softKnee;

    float bloomAmount;
    int lowerQuality;
    int tonemappingType;
    int applyGamma;
} AVD_BloomUberPushConstants;

static bool __avdBloomCreateFramebuffers(AVD_Bloom *bloom, AVD_Vulkan *vulkan, uint32_t width, uint32_t height)
{
    AVD_ASSERT(bloom != NULL);
    AVD_ASSERT(vulkan != NULL);

    // down sampling passes
    for (uint32_t i = 0; i < bloom->passCount - 1; ++i) {
        uint32_t framebufferWidth  = width >> (i + 1);
        uint32_t framebufferHeight = height >> (i + 1);
        // AVD_LOG("Down Pass %d: %dx%d\n", i, framebufferWidth, framebufferHeight);
        AVD_VulkanFramebuffer *framebuffer = &bloom->bloomPasses[i];
        AVD_CHECK(avdVulkanFramebufferCreate(
            vulkan,
            framebuffer,
            framebufferWidth,
            framebufferHeight,
            false,
            (VkFormat[]){VK_FORMAT_R16G16B16A16_SFLOAT},
            1,
            VK_FORMAT_D32_SFLOAT_S8_UINT));
    }

    // up sampling passes
    for (uint32_t i = bloom->passCount - 1; i < bloom->passCount * 2 - 2; ++i) {
        uint32_t framebufferWidth  = width >> (2 * bloom->passCount - 3 - i);
        uint32_t framebufferHeight = height >> (2 * bloom->passCount - 3 - i);
        // AVD_LOG("Up Pass %d: %dx%d\n", i, framebufferWidth, framebufferHeight);
        AVD_VulkanFramebuffer *framebuffer = &bloom->bloomPasses[i];
        AVD_CHECK(avdVulkanFramebufferCreate(
            vulkan,
            framebuffer,
            framebufferWidth,
            framebufferHeight,
            false,
            (VkFormat[]){VK_FORMAT_R16G16B16A16_SFLOAT},
            1,
            VK_FORMAT_D32_SFLOAT_S8_UINT));
    }

    // // print the width and height of all the framebuffers
    // for (uint32_t i = 0; i < bloom->passCount * 2 - 2; ++i) {
    //     AVD_VulkanFramebuffer *framebuffer = &bloom->bloomPasses[i];
    //     AVD_LOG("Framebuffer %d: %dx%d\n", i, framebuffer->width, framebuffer->height);
    // }

    return true;
}

static bool __avdBloomPass(
    VkCommandBuffer commandBuffer,
    AVD_BloomPassType passType,
    AVD_Bloom *bloom,
    AVD_Vulkan *vulkan,
    AVD_VulkanFramebuffer *srcFramebuffer,
    uint32_t sourceIndex,
    uint32_t targetIndex,
    AVD_BloomParams params)
{
    AVD_ASSERT(bloom != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    VkDescriptorSet customTexture0 = VK_NULL_HANDLE;
    VkDescriptorSet customTexture1 = VK_NULL_HANDLE;

    switch (passType) {
        case AVD_BLOOM_PASS_TYPE_PREFILTER:
        case AVD_BLOOM_PASS_TYPE_DOWNSAMPLE_PREFILTER:
            customTexture0 = avdVulkanFramebufferGetColorAttachment(srcFramebuffer, 0)->descriptorSet;
            // unused, TODO: use a fallback image descriptor set here
            customTexture1 = avdVulkanFramebufferGetColorAttachment(&bloom->bloomPasses[1], 0)->descriptorSet;
            break;
        case AVD_BLOOM_PASS_TYPE_DOWNSAMPLE:
            customTexture0 = avdVulkanFramebufferGetColorAttachment(&bloom->bloomPasses[sourceIndex], 0)->descriptorSet;
            customTexture1 = avdVulkanFramebufferGetColorAttachment(&bloom->bloomPasses[0], 0)->descriptorSet;
            break;
        case AVD_BLOOM_PASS_TYPE_UPSAMPLE:
            customTexture0 = avdVulkanFramebufferGetColorAttachment(&bloom->bloomPasses[sourceIndex], 0)->descriptorSet;
            customTexture1 = avdVulkanFramebufferGetColorAttachment(&bloom->bloomPasses[2 * bloom->passCount - 5 - sourceIndex], 0)->descriptorSet;
            break;
        case AVD_BLOOM_PASS_TYPE_COMPOSITE:
            customTexture0 = avdVulkanFramebufferGetColorAttachment(&bloom->bloomPasses[2 * bloom->passCount - 4], 0)->descriptorSet;
            customTexture1 = avdVulkanFramebufferGetColorAttachment(&bloom->bloomPasses[2 * bloom->passCount - 3], 0)->descriptorSet;
            break;
        default:
            AVD_ASSERT(false);
    }

    VkDescriptorSet descriptorSets[] = {
        customTexture0,
        customTexture1,
    };

    VkPipeline targetPipeline                = bloom->pipeline;
    AVD_VulkanFramebuffer *targetFramebuffer = &bloom->bloomPasses[targetIndex];
    if (passType == AVD_BLOOM_PASS_TYPE_COMPOSITE) {
        targetFramebuffer = srcFramebuffer;
        targetPipeline    = bloom->pipelineComposite;
    }

    static VkImageView attachments[16] = {0};
    static size_t attachmentCount      = 0;
    AVD_CHECK(avdVulkanFramebufferGetAttachmentViews(
        targetFramebuffer,
        attachments,
        &attachmentCount));

    AVD_CHECK(avdBeginRenderPass(
        commandBuffer,
        targetFramebuffer->renderPass,
        targetFramebuffer->framebuffer,
        attachments,
        attachmentCount,
        targetFramebuffer->width,
        targetFramebuffer->height,
        NULL, 0));

    AVD_BloomUberPushConstants pushConstants = {
        .type                    = passType,
        .srcWidth                = (float)bloom->bloomPasses[sourceIndex].width,
        .srcHeight               = (float)bloom->bloomPasses[sourceIndex].height,
        .targetWidth             = (float)targetFramebuffer->width,
        .targetHeight            = (float)targetFramebuffer->height,
        .bloomPrefilterType      = (int)params.prefilterType,
        .bloomPrefilterThreshold = params.threshold,
        .softKnee                = params.softKnee,
        .bloomAmount             = params.bloomAmount,
        .lowerQuality            = params.lowQuality ? 1 : 0,
        .tonemappingType         = (int)params.tonemappingType,
        .applyGamma              = params.applyGamma ? 1 : 0,
    };

    vkCmdPushConstants(commandBuffer, bloom->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(AVD_BloomUberPushConstants), &pushConstants);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, targetPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloom->pipelineLayout, 0, AVD_ARRAY_COUNT(descriptorSets), descriptorSets, 0, NULL);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    AVD_CHECK(avdEndRenderPass(commandBuffer));
    return true;
}

bool avdBloomCreate(AVD_Bloom *bloom, AVD_Vulkan *vulkan, VkRenderPass compositeRenderPass, uint32_t width, uint32_t height)
{
    // build the framebuffers
    bloom->width     = width;
    bloom->height    = height;
    bloom->passCount = AVD_BLOOM_PASS_COUNT;

    AVD_CHECK(__avdBloomCreateFramebuffers(bloom, vulkan, width, height));

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &bloom->bloomDescriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));
    AVD_CHECK(avdPipelineUtilsCreateGraphicsPipelineLayout(
        &bloom->pipelineLayout,
        vulkan->device,
        (VkDescriptorSetLayout[]){
            bloom->bloomDescriptorSetLayout,
            bloom->bloomDescriptorSetLayout},
        2,
        sizeof(AVD_BloomUberPushConstants)));
    AVD_CHECK(avdPipelineUtilsCreateGenericGraphicsPipeline(
        &bloom->pipeline,
        bloom->pipelineLayout,
        vulkan->device,
        bloom->bloomPasses[0].renderPass,
        1,
        "FullScreenQuadVert",
        "BloomFrag",
        NULL,
        NULL));

    AVD_CHECK(avdPipelineUtilsCreateGenericGraphicsPipeline(
        &bloom->pipelineComposite,
        bloom->pipelineLayout,
        vulkan->device,
        compositeRenderPass,
        1,
        "FullScreenQuadVert",
        "BloomFrag",
        NULL,
        NULL));

    return true;
}

void avdBloomDestroy(AVD_Bloom *bloom, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(bloom != NULL);
    AVD_ASSERT(vulkan != NULL);

    for (uint32_t i = 0; i < bloom->passCount * 2 - 1; ++i) {
        avdVulkanFramebufferDestroy(vulkan, &bloom->bloomPasses[i]);
    }
    vkDestroyPipeline(vulkan->device, bloom->pipelineComposite, NULL);
    vkDestroyPipeline(vulkan->device, bloom->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, bloom->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, bloom->bloomDescriptorSetLayout, NULL);
}

bool avdBloomApplyInplace(
    VkCommandBuffer commandBuffer,
    AVD_Bloom *bloom,
    AVD_VulkanFramebuffer *inputFramebuffer,
    AVD_Vulkan *vulkan,
    AVD_BloomParams params)
{
    AVD_ASSERT(bloom != NULL);
    AVD_ASSERT(inputFramebuffer != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_CHECK(__avdBloomPass(
        commandBuffer,
        AVD_BLOOM_PASS_TYPE_DOWNSAMPLE_PREFILTER,
        bloom, vulkan,
        inputFramebuffer,
        0, // source index (can be anything for prefilter pass)
        0, // target index
        params));

    for (uint32_t i = 0; i < bloom->passCount - 2; ++i) {
        // AVD_LOG("Downsampling pass %d -> %d\n", i, i + 1);
        AVD_CHECK(__avdBloomPass(
            commandBuffer,
            AVD_BLOOM_PASS_TYPE_DOWNSAMPLE,
            bloom, vulkan,
            inputFramebuffer,
            i,     // source index
            i + 1, // target index
            params));
    }

    for (uint32_t i = bloom->passCount - 2; i < bloom->passCount * 2 - 4; i++) {
        // AVD_LOG("Upsampling pass %d -> %d\n", i, i + 1);
        AVD_CHECK(__avdBloomPass(
            commandBuffer,
            AVD_BLOOM_PASS_TYPE_UPSAMPLE,
            bloom, vulkan,
            inputFramebuffer,
            i,     // source index
            i + 1, // target index
            params));
    }

    // another prefiltering without theshold
    params.prefilterType = AVD_BLOOM_PREFILTER_TYPE_NONE;
    AVD_CHECK(__avdBloomPass(
        commandBuffer,
        AVD_BLOOM_PASS_TYPE_PREFILTER,
        bloom, vulkan,
        inputFramebuffer,
        0,                        // source index (can be anything for prefilter pass)
        bloom->passCount * 2 - 3, // target index
        params));

    AVD_CHECK(__avdBloomPass(
        commandBuffer,
        AVD_BLOOM_PASS_TYPE_COMPOSITE,
        bloom, vulkan,
        inputFramebuffer,
        0, // source index
        0, // target index
        params));

    return true;
}

const char *avdBloomPassTypeToString(AVD_BloomPassType type)
{
    switch (type) {
        case AVD_BLOOM_PASS_TYPE_PREFILTER:
            return "Prefilter";
        case AVD_BLOOM_PASS_TYPE_DOWNSAMPLE:
            return "Downsample";
        case AVD_BLOOM_PASS_TYPE_UPSAMPLE:
            return "Upsample";
        case AVD_BLOOM_PASS_TYPE_COMPOSITE:
            return "Composite";
        case AVD_BLOOM_PASS_TYPE_DOWNSAMPLE_PREFILTER:
            return "Downsample_Prefilter";
        default:
            return "Unknown";
    }
}

const char *avdBloomPrefilterTypeToString(AVD_BloomPrefilterType type)
{
    switch (type) {
        case AVD_BLOOM_PREFILTER_TYPE_NONE:
            return "None";
        case AVD_BLOOM_PREFILTER_TYPE_THRESHOLD:
            return "Threshold";
        case AVD_BLOOM_PREFILTER_TYPE_SOFTKNEE:
            return "SoftKnee";
        default:
            return "Unknown";
    }
}