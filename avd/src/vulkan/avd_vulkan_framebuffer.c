#include "vulkan/avd_vulkan_framebuffer.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"

static bool __avdVulkanFramebufferAttachmentDescriptorsCreate(AVD_Vulkan *vulkan, AVD_VulkanFramebufferAttachment *attachment)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(attachment != NULL);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {0};
    descriptorSetLayoutBinding.binding                      = 0;
    descriptorSetLayoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBinding.descriptorCount              = 1;
    descriptorSetLayoutBinding.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {0};
    descriptorSetLayoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount                    = 1;
    descriptorSetLayoutInfo.pBindings                       = &descriptorSetLayoutBinding;

    VkResult result = vkCreateDescriptorSetLayout(vulkan->device, &descriptorSetLayoutInfo, NULL, &attachment->descriptorSetLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create framebuffer attachment descriptor set layout");

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {0};
    descriptorSetAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool              = vulkan->descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount          = 1;
    descriptorSetAllocateInfo.pSetLayouts                 = &attachment->descriptorSetLayout;

    result = vkAllocateDescriptorSets(vulkan->device, &descriptorSetAllocateInfo, &attachment->descriptorSet);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate framebuffer attachment descriptor set");

    VkWriteDescriptorSet writeDescriptorSet = {0};
    AVD_CHECK(avdWriteImageDescriptorSet(&writeDescriptorSet, attachment->descriptorSet, 0, &attachment->image.descriptorImageInfo));
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

    AVD_CHECK(avdVulkanImageCreate(vulkan, &attachment->image, format, usage, width, height));
    AVD_CHECK(__avdVulkanFramebufferAttachmentDescriptorsCreate(vulkan, attachment));

    attachment->attachmentDescription.flags          = 0;
    attachment->attachmentDescription.format         = attachment->image.format;
    attachment->attachmentDescription.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment->attachmentDescription.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment->attachmentDescription.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment->attachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment->attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment->attachmentDescription.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;

    if (avdVulkanFormatIsDepthStencil(format))
        attachment->attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    else
        attachment->attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    attachment->attachmentImageInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
    attachment->attachmentImageInfo.flags           = 0;
    attachment->attachmentImageInfo.usage           = usage;
    attachment->attachmentImageInfo.width           = width;
    attachment->attachmentImageInfo.height          = height;
    attachment->attachmentImageInfo.layerCount      = 1;
    attachment->attachmentImageInfo.viewFormatCount = 1;
    attachment->attachmentImageInfo.pViewFormats    = &attachment->image.format;
    attachment->attachmentImageInfo.pNext           = NULL;

    return true;
}

static bool __avdVulkanFramebufferCreateRenderPassAndFramebuffer(VkDevice device, AVD_VulkanFramebuffer *framebuffer)
{
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(framebuffer != NULL);

    static VkAttachmentDescription colorAttachmentDescriptions[64] = {0};
    uint32_t attachmentCount                                       = (uint32_t)framebuffer->colorAttachments.count;
    if (attachmentCount > (uint32_t)AVD_ARRAY_COUNT(colorAttachmentDescriptions)) {
        AVD_LOG("Too many color attachments for framebuffer, max is %zu", AVD_ARRAY_COUNT(colorAttachmentDescriptions));
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
        colorAttachmentReferences[i].attachment = (uint32_t)i;
        colorAttachmentReferences[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    if (framebuffer->hasDepthStencil) {
        colorAttachmentReferences[attachmentCount - 1].attachment = (uint32_t)framebuffer->colorAttachments.count;
        colorAttachmentReferences[attachmentCount - 1].layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpassDescription = {0};
    subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = (uint32_t)framebuffer->colorAttachments.count;
    subpassDescription.pColorAttachments    = colorAttachmentReferences;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments    = NULL;
    subpassDescription.pResolveAttachments  = NULL;
    if (framebuffer->hasDepthStencil)
        subpassDescription.pDepthStencilAttachment = &colorAttachmentReferences[attachmentCount - 1];
    else
        subpassDescription.pDepthStencilAttachment = NULL;

    static VkSubpassDependency dependencies[2] = {0};

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = attachmentCount;
    renderPassInfo.pAttachments           = colorAttachmentDescriptions;
    renderPassInfo.subpassCount           = 1;
    renderPassInfo.pSubpasses             = &subpassDescription;
    renderPassInfo.dependencyCount        = 2;
    renderPassInfo.pDependencies          = dependencies;
    VkResult result                       = vkCreateRenderPass(device, &renderPassInfo, NULL, &framebuffer->renderPass);
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

    VkFramebufferAttachmentsCreateInfo framebufferAttachmentsInfo = {0};
    framebufferAttachmentsInfo.sType                              = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
    framebufferAttachmentsInfo.pNext                              = NULL;
    framebufferAttachmentsInfo.attachmentImageInfoCount           = attachmentCount;
    framebufferAttachmentsInfo.pAttachmentImageInfos              = attachmentImageInfos;

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags                   = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
    framebufferInfo.renderPass              = framebuffer->renderPass;
    framebufferInfo.attachmentCount         = attachmentCount;
    framebufferInfo.pAttachments            = NULL;
    framebufferInfo.width                   = framebuffer->width;
    framebufferInfo.height                  = framebuffer->height;
    framebufferInfo.layers                  = 1;
    framebufferInfo.pNext                   = &framebufferAttachmentsInfo;
    result                                  = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffer->framebuffer);
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
            AVD_LOG("Failed to create color attachment for format %d\n", colorFormats[i]);
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
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)) {
            AVD_LOG("Failed to transition color attachment image layout\n");
            return false;
        }
    }

    if (hasDepthStencil) {
        if (!__avdVulkanFramebufferAttachmentCreate(vulkan, &framebuffer->depthStencilAttachment, depthStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height)) {
            AVD_LOG("Failed to create depth stencil attachment\n");
            return false;
        }
    }

    // Create render pass and framebuffer here (omitted for brevity)
    if (!__avdVulkanFramebufferCreateRenderPassAndFramebuffer(vulkan->device, framebuffer)) {
        AVD_LOG("Failed to create render pass and framebuffer\n");
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
        colorAttachmentView[i]                      = attachment->image.imageView;
    }
    *attachmentCount = framebuffer->colorAttachments.count;

    if (framebuffer->hasDepthStencil) {
        colorAttachmentView[*attachmentCount] = framebuffer->depthStencilAttachment.image.imageView;
        *attachmentCount += 1;
    }

    return true;
}