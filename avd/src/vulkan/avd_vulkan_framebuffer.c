#include "vulkan/avd_vulkan_framebuffer.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"

static bool __avdVulkanFramebufferAttachmentDescriptorsCreate(AVD_Vulkan *vulkan, AVD_VulkanFramebufferAttachment *attachment)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(attachment != NULL);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
        .binding         = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings    = &descriptorSetLayoutBinding,
    };

    VkResult result = vkCreateDescriptorSetLayout(vulkan->device, &descriptorSetLayoutInfo, NULL, &attachment->descriptorSetLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create framebuffer attachment descriptor set layout");

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = vulkan->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &attachment->descriptorSetLayout,
    };

    result = vkAllocateDescriptorSets(vulkan->device, &descriptorSetAllocateInfo, &attachment->descriptorSet);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate framebuffer attachment descriptor set");

    VkWriteDescriptorSet writeDescriptorSet = {0};
    AVD_CHECK(avdWriteImageDescriptorSet(&writeDescriptorSet, attachment->descriptorSet, 0, &attachment->image.defaultSubresource.descriptorImageInfo));
    vkUpdateDescriptorSets(vulkan->device, 1, &writeDescriptorSet, 0, NULL);

    return true;
}

static void __avdVulkanFramebufferAttachmentDestroy(AVD_Vulkan *vulkan, AVD_VulkanFramebufferAttachment *attachment)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(attachment != NULL);

    avdVulkanImageDestroy(vulkan, &attachment->image);
    vkDestroyDescriptorSetLayout(vulkan->device, attachment->descriptorSetLayout, NULL);
}

static bool __avdVulkanFramebufferAttachmentCreate(AVD_Vulkan *vulkan, AVD_VulkanFramebufferAttachment *attachment, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(attachment != NULL);

    AVD_CHECK(avdVulkanImageCreate(vulkan, &attachment->image,
                                   avdVulkanImageGetDefaultCreateInfo(
                                       width,
                                       height,
                                       format,
                                       usage, "Core/FramebufferAttachment")));
    AVD_CHECK(__avdVulkanFramebufferAttachmentDescriptorsCreate(vulkan, attachment));

    attachment->attachmentDescription = (VkAttachmentDescription){
        .flags          = 0,
        .format         = attachment->image.info.format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = avdVulkanFormatIsDepthStencil(format)
                              ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                              : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    attachment->attachmentImageInfo = (VkFramebufferAttachmentImageInfo){
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
        .pNext           = NULL,
        .flags           = 0,
        .usage           = usage,
        .width           = width,
        .height          = height,
        .layerCount      = 1,
        .viewFormatCount = 1,
        .pViewFormats    = &attachment->image.info.format,
    };

    return true;
}

static bool __avdVulkanFramebufferCreateRenderPassAndFramebuffer(VkDevice device, AVD_VulkanFramebuffer *framebuffer)
{
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(framebuffer != NULL);

    static VkAttachmentDescription colorAttachmentDescriptions[64] = {0};
    uint32_t attachmentCount                                       = (uint32_t)framebuffer->colorAttachments.count;
    if (attachmentCount > (uint32_t)AVD_ARRAY_COUNT(colorAttachmentDescriptions)) {
        AVD_LOG_ERROR("Too many color attachments for framebuffer, max is %zu", AVD_ARRAY_COUNT(colorAttachmentDescriptions));
        return false;
    }
    for (size_t i = 0; i < framebuffer->colorAttachments.count; ++i) {
        AVD_VulkanFramebufferAttachment *attachment = (AVD_VulkanFramebufferAttachment *)avdListGet(&framebuffer->colorAttachments, i);
        colorAttachmentDescriptions[i]              = attachment->attachmentDescription;
    }
    if (framebuffer->hasDepthStencil) {
        colorAttachmentDescriptions[attachmentCount] = framebuffer->depthStencilAttachment.attachmentDescription;
        attachmentCount += 1;
    }

    static VkAttachmentReference colorAttachmentReferences[64] = {0};
    uint32_t referenceCount                                    = attachmentCount;
    for (size_t i = 0; i < framebuffer->colorAttachments.count; ++i) {
        colorAttachmentReferences[i] = (VkAttachmentReference){
            .attachment = (uint32_t)i,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
    }

    if (framebuffer->hasDepthStencil) {
        colorAttachmentReferences[attachmentCount - 1] = (VkAttachmentReference){
            .attachment = (uint32_t)framebuffer->colorAttachments.count,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    VkSubpassDescription subpassDescription = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = (uint32_t)framebuffer->colorAttachments.count,
        .pColorAttachments       = colorAttachmentReferences,
        .inputAttachmentCount    = 0,
        .pInputAttachments       = NULL,
        .pResolveAttachments     = NULL,
        .pDepthStencilAttachment = framebuffer->hasDepthStencil
                                       ? &colorAttachmentReferences[attachmentCount - 1]
                                       : NULL,
    };

    static VkSubpassDependency dependencies[2] = {0};

    dependencies[0] = (VkSubpassDependency){
        .srcSubpass      = VK_SUBPASS_EXTERNAL,
        .dstSubpass      = 0,
        .srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    };

    dependencies[1] = (VkSubpassDependency){
        .srcSubpass      = 0,
        .dstSubpass      = VK_SUBPASS_EXTERNAL,
        .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachmentCount,
        .pAttachments    = colorAttachmentDescriptions,
        .subpassCount    = 1,
        .pSubpasses      = &subpassDescription,
        .dependencyCount = 2,
        .pDependencies   = dependencies,
    };
    VkResult result = vkCreateRenderPass(device, &renderPassInfo, NULL, &framebuffer->renderPass);
    AVD_CHECK_VK_RESULT(result, "Failed to create render pass");

    static VkFramebufferAttachmentImageInfo attachmentImageInfos[64] = {0};
    // attachmentImageInfos[0]                                          = framebuffer->colorAttachment.attachmentImageInfo;
    for (size_t i = 0; i < framebuffer->colorAttachments.count; ++i) {
        AVD_VulkanFramebufferAttachment *attachment = (AVD_VulkanFramebufferAttachment *)avdListGet(&framebuffer->colorAttachments, i);
        attachmentImageInfos[i]                     = attachment->attachmentImageInfo;
    }
    if (framebuffer->hasDepthStencil) {
        attachmentImageInfos[attachmentCount - 1] = framebuffer->depthStencilAttachment.attachmentImageInfo;
    }

    VkFramebufferAttachmentsCreateInfo framebufferAttachmentsInfo = {
        .sType                    = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
        .pNext                    = NULL,
        .attachmentImageInfoCount = attachmentCount,
        .pAttachmentImageInfos    = attachmentImageInfos,
    };

    VkFramebufferCreateInfo framebufferInfo = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .flags           = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
        .renderPass      = framebuffer->renderPass,
        .attachmentCount = attachmentCount,
        .pAttachments    = NULL,
        .width           = framebuffer->width,
        .height          = framebuffer->height,
        .layers          = 1,
        .pNext           = &framebufferAttachmentsInfo,
    };
    result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffer->framebuffer);
    AVD_CHECK_VK_RESULT(result, "Failed to create framebuffer");

    return true;
}

bool avdVulkanFormatIsDepth(VkFormat format)
{
    const VkFormat depthFormats[] = {
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT};
    const size_t depthFormatCount = sizeof(depthFormats) / sizeof(depthFormats[0]);

    bool isDepth = false;
    for (size_t i = 0; i < depthFormatCount; ++i) {
        if (format == depthFormats[i]) {
            isDepth = true;
            break;
        }
    }

    return isDepth;
}

bool avdVulkanFormatIsStencil(VkFormat format)
{
    const VkFormat stencilFormats[] = {
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT};
    const size_t stencilFormatCount = sizeof(stencilFormats) / sizeof(stencilFormats[0]);

    bool isStencil = false;
    for (size_t i = 0; i < stencilFormatCount; ++i) {
        if (format == stencilFormats[i]) {
            isStencil = true;
            break;
        }
    }

    return isStencil;
}

bool avdVulkanFormatIsDepthStencil(VkFormat format)
{
    const bool isDepth   = avdVulkanFormatIsDepth(format);
    const bool isStencil = avdVulkanFormatIsStencil(format);
    return isDepth || isStencil;
}

bool avdVulkanFramebufferCreate(
    AVD_Vulkan *vulkan,
    AVD_VulkanFramebuffer *framebuffer,
    int32_t width,
    int32_t height,
    bool hasDepthStencil,
    VkFormat *colorFormats,
    uint32_t formatCount,
    VkFormat depthStencilFormat)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(framebuffer != NULL);
    AVD_CHECK_MSG(width > 0, "Framebuffer width must be greater than 0");
    AVD_CHECK_MSG(height > 0, "Framebuffer height must be greater than 0");

    framebuffer->width           = width;
    framebuffer->height          = height;
    framebuffer->hasDepthStencil = hasDepthStencil;

    avdListCreate(&framebuffer->colorAttachments, sizeof(AVD_VulkanFramebufferAttachment));

    for (uint32_t i = 0; i < formatCount; ++i) {
        AVD_VulkanFramebufferAttachment attachment     = {0};
        AVD_VulkanFramebufferAttachment *attachmentPtr = (AVD_VulkanFramebufferAttachment *)avdListPushBack(&framebuffer->colorAttachments, &attachment);
        if (!__avdVulkanFramebufferAttachmentCreate(vulkan, attachmentPtr, colorFormats[i], VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height)) {
            AVD_LOG_ERROR("Failed to create color attachment for format %d", colorFormats[i]);
            return false;
        }
    }

    for (size_t i = 0; i < framebuffer->colorAttachments.count; ++i) {
        AVD_VulkanFramebufferAttachment *attachment = (AVD_VulkanFramebufferAttachment *)avdListGet(&framebuffer->colorAttachments, i);
        if (!avdVulkanImageTransitionLayoutWithoutCommandBuffer(
                vulkan,
                &attachment->image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                NULL)) {
            AVD_LOG_ERROR("Failed to transition color attachment image layout");
            return false;
        }
    }

    if (hasDepthStencil) {
        if (!__avdVulkanFramebufferAttachmentCreate(vulkan, &framebuffer->depthStencilAttachment, depthStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height)) {
            AVD_LOG_ERROR("Failed to create depth stencil attachment");
            return false;
        }
    }

    // Create render pass and framebuffer here (omitted for brevity)
    if (!__avdVulkanFramebufferCreateRenderPassAndFramebuffer(vulkan->device, framebuffer)) {
        AVD_LOG_ERROR("Failed to create render pass and framebuffer");
        return false;
    }

    return true;
}

void avdVulkanFramebufferDestroy(AVD_Vulkan *vulkan, AVD_VulkanFramebuffer *framebuffer)
{
    for (size_t i = 0; i < framebuffer->colorAttachments.count; ++i) {
        AVD_VulkanFramebufferAttachment *attachment = (AVD_VulkanFramebufferAttachment *)avdListGet(&framebuffer->colorAttachments, i);
        __avdVulkanFramebufferAttachmentDestroy(vulkan, attachment);
    }

    if (framebuffer->hasDepthStencil) {
        __avdVulkanFramebufferAttachmentDestroy(vulkan, &framebuffer->depthStencilAttachment);
    }
    vkDestroyRenderPass(vulkan->device, framebuffer->renderPass, NULL);
    vkDestroyFramebuffer(vulkan->device, framebuffer->framebuffer, NULL);
    avdListDestroy(&framebuffer->colorAttachments);
}

AVD_VulkanFramebufferAttachment *avdVulkanFramebufferGetColorAttachment(AVD_VulkanFramebuffer *framebuffer, size_t index)
{
    AVD_ASSERT(framebuffer != NULL);
    AVD_ASSERT(index < framebuffer->colorAttachments.count);

    return (AVD_VulkanFramebufferAttachment *)avdListGet(&framebuffer->colorAttachments, index);
}

bool avdVulkanFramebufferGetAttachmentViews(AVD_VulkanFramebuffer *framebuffer, VkImageView *colorAttachmentView, size_t *attachmentCount)
{
    AVD_ASSERT(framebuffer != NULL);
    AVD_ASSERT(colorAttachmentView != NULL);
    AVD_ASSERT(attachmentCount != NULL);

    for (size_t i = 0; i < framebuffer->colorAttachments.count; ++i) {
        AVD_VulkanFramebufferAttachment *attachment = avdVulkanFramebufferGetColorAttachment(framebuffer, i);
        colorAttachmentView[i]                      = attachment->image.defaultSubresource.imageView;
    }
    *attachmentCount = framebuffer->colorAttachments.count;

    if (framebuffer->hasDepthStencil) {
        colorAttachmentView[*attachmentCount] = framebuffer->depthStencilAttachment.image.defaultSubresource.imageView;
        *attachmentCount += 1;
    }

    return true;
}