#include "ps_vulkan.h"

static bool __psVulkanFramebufferAttachmentDescriptorsCreate(PS_Vulkan *vulkan, PS_VulkanFramebufferAttachment *attachment)
{
    PS_ASSERT(vulkan != NULL);
    PS_ASSERT(attachment != NULL);
    
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {0};
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {0};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = 1;
    descriptorSetLayoutInfo.pBindings = &descriptorSetLayoutBinding;

    VkResult result = vkCreateDescriptorSetLayout(vulkan->device, &descriptorSetLayoutInfo, NULL, &attachment->descriptorSetLayout);
    PS_CHECK_VK_RESULT(result, "Failed to create framebuffer attachment descriptor set layout");

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {0};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;   
    descriptorSetAllocateInfo.descriptorPool = vulkan->descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &attachment->descriptorSetLayout;
    
    result = vkAllocateDescriptorSets(vulkan->device, &descriptorSetAllocateInfo, &attachment->descriptorSet);
    PS_CHECK_VK_RESULT(result, "Failed to allocate framebuffer attachment descriptor set");

    VkWriteDescriptorSet writeDescriptorSet = {0};
    PS_CHECK(psWriteImageDescriptorSet(&writeDescriptorSet, attachment->descriptorSet, 0, &attachment->image.descriptorImageInfo));
    vkUpdateDescriptorSets(vulkan->device, 1, &writeDescriptorSet, 0, NULL);

    return true;
}

static void __psVulkanFramebufferAttachmentDestroy(PS_Vulkan *vulkan, PS_VulkanFramebufferAttachment *attachment)
{
    PS_ASSERT(vulkan != NULL);
    PS_ASSERT(attachment != NULL);

    psVulkanImageDestroy(vulkan, &attachment->image);
    vkDestroyDescriptorSetLayout(vulkan->device, attachment->descriptorSetLayout, NULL);
}

static bool __psVulkanFramebufferAttachmentCreate(PS_Vulkan *vulkan, PS_VulkanFramebufferAttachment *attachment, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height)
{
    PS_ASSERT(vulkan != NULL);
    PS_ASSERT(attachment != NULL);

    PS_CHECK(psVulkanImageCreate(vulkan, &attachment->image, format, usage, width, height));
    PS_CHECK(__psVulkanFramebufferAttachmentDescriptorsCreate(vulkan, attachment));

    attachment->attachmentDescription.flags = 0;
    attachment->attachmentDescription.format = format;
    attachment->attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment->attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment->attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment->attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment->attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment->attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (psVulkanFormatIsDepthStencil(format))
        attachment->attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    else
        attachment->attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return true;
}

static bool __psVulkanFramebufferCreateRenderPassAndFramebuffer(VkDevice device, PS_VulkanFramebuffer *framebuffer)
{
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(framebuffer != NULL);

    static VkAttachmentDescription colorAttachmentDescriptions[16] = {0};
    uint32_t attachmentCount = 1;
    colorAttachmentDescriptions[0] = framebuffer->colorAttachment.attachmentDescription;
    if (framebuffer->hasDepthStencil)
    {
        attachmentCount += 1;
        colorAttachmentDescriptions[1] = framebuffer->depthStencilAttachment.attachmentDescription;
    }

    static VkAttachmentReference colorAttachmentReferences[16] = {0};
    uint32_t referenceCount = 1;

    colorAttachmentReferences[0].attachment = 0;
    colorAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (framebuffer->hasDepthStencil)
    {
        referenceCount += 1;
        colorAttachmentReferences[1].attachment = 1;
        colorAttachmentReferences[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpassDescription = {0};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReferences[0];
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = NULL;
    subpassDescription.pResolveAttachments = NULL;
    if (framebuffer->hasDepthStencil)
        subpassDescription.pDepthStencilAttachment = &colorAttachmentReferences[1];
    else
        subpassDescription.pDepthStencilAttachment = NULL;

    static VkSubpassDependency dependencies[2] = {0};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachmentCount;
    renderPassInfo.pAttachments = colorAttachmentDescriptions;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;
    VkResult result = vkCreateRenderPass(device, &renderPassInfo, NULL, &framebuffer->renderPass);
    PS_CHECK_VK_RESULT(result, "Failed to create render pass");

    static VkImageView attachmentViews[16] = {0};
    attachmentViews[0] = framebuffer->colorAttachment.image.imageView;
    if (framebuffer->hasDepthStencil)
        attachmentViews[1] = framebuffer->depthStencilAttachment.image.imageView;

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = framebuffer->renderPass;
    framebufferInfo.attachmentCount = attachmentCount;
    framebufferInfo.pAttachments = attachmentViews;
    framebufferInfo.width = framebuffer->width;
    framebufferInfo.height = framebuffer->height;
    framebufferInfo.layers = 1;
    result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffer->framebuffer);
    PS_CHECK_VK_RESULT(result, "Failed to create framebuffer");

    return true;
}

bool psVulkanFormatIsDepth(VkFormat format)
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
    for (size_t i = 0; i < depthFormatCount; ++i)
    {
        if (format == depthFormats[i])
        {
            isDepth = true;
            break;
        }
    }

    return isDepth;
}

bool psVulkanFormatIsStencil(VkFormat format)
{
    const VkFormat stencilFormats[] = {
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT};
    const size_t stencilFormatCount = sizeof(stencilFormats) / sizeof(stencilFormats[0]);

    bool isStencil = false;
    for (size_t i = 0; i < stencilFormatCount; ++i)
    {
        if (format == stencilFormats[i])
        {
            isStencil = true;
            break;
        }
    }

    return isStencil;
}

bool psVulkanFormatIsDepthStencil(VkFormat format)
{
    const bool isDepth = psVulkanFormatIsDepth(format);
    const bool isStencil = psVulkanFormatIsStencil(format);
    return isDepth || isStencil;
}

bool psVulkanFramebufferCreate(PS_Vulkan *vulkan, PS_VulkanFramebuffer *framebuffer, int32_t width, int32_t height, bool hasDepthStencil, VkFormat colorFormat, VkFormat depthStencilFormat)
{
    PS_ASSERT(vulkan != NULL);
    PS_ASSERT(framebuffer != NULL);

    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->hasDepthStencil = hasDepthStencil;

    if (!__psVulkanFramebufferAttachmentCreate(vulkan, &framebuffer->colorAttachment, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height))
    {
        PS_LOG("Failed to create color attachment\n");
        return false;
    }

    // transition image layout to SHADER_READ_ONLY_OPTIMAL
    if (!psVulkanImageTransitionLayoutWithoutCommandBuffer(
            vulkan,
            &framebuffer->colorAttachment.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT))
    {
        PS_LOG("Failed to transition image layout\n");
        return false;
    }

    if (hasDepthStencil)
    {
        if (!__psVulkanFramebufferAttachmentCreate(vulkan, &framebuffer->depthStencilAttachment, depthStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, width, height))
        {
            PS_LOG("Failed to create depth stencil attachment\n");
            return false;
        }
    }

    // Create render pass and framebuffer here (omitted for brevity)
    if (!__psVulkanFramebufferCreateRenderPassAndFramebuffer(vulkan->device, framebuffer))
    {
        PS_LOG("Failed to create render pass and framebuffer\n");
        return false;
    }

    return true;
}

void psVulkanFramebufferDestroy(PS_Vulkan *vulkan, PS_VulkanFramebuffer *framebuffer)
{
    __psVulkanFramebufferAttachmentDestroy(vulkan, &framebuffer->colorAttachment);
    if (framebuffer->hasDepthStencil)
    {
        __psVulkanFramebufferAttachmentDestroy(vulkan, &framebuffer->depthStencilAttachment);
    }
    vkDestroyRenderPass(vulkan->device, framebuffer->renderPass, NULL);
    vkDestroyFramebuffer(vulkan->device, framebuffer->framebuffer, NULL);
}