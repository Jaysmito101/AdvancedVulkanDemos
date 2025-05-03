#include "ps_vulkan.h"

static void __psVulkanFramebufferAttachmentDestroy(PS_GameState *gameState, PS_VulkanFramebufferAttachment *attachment)
{
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(attachment != NULL);

    vkDestroyImageView(gameState->vulkan.device, attachment->imageView, NULL);
    attachment->imageView = VK_NULL_HANDLE;

    vkDestroyImage(gameState->vulkan.device, attachment->image, NULL);
    attachment->image = VK_NULL_HANDLE;

    vkFreeMemory(gameState->vulkan.device, attachment->memory, NULL);
    attachment->memory = VK_NULL_HANDLE;
}

static bool __psVulkanFramebufferAttachmentCreate(PS_GameState *gameState, PS_VulkanFramebufferAttachment *attachment, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height)
{
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(attachment != NULL);

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_NONE;

    attachment->format = format;
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        if (psVulkanFramebufferAttachmentIsDepth(attachment))
        {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (psVulkanFramebufferAttachmentIsStencil(attachment))
        {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        PS_LOG("Invalid image usage flags\n");
        return false;
    }

    PS_ASSERT(aspectMask != VK_IMAGE_ASPECT_NONE);

    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(gameState->vulkan.device, &imageInfo, NULL, &attachment->image);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create image\n");
        return false;
    }

    VkMemoryRequirements memRequirements = {0};
    vkGetImageMemoryRequirements(gameState->vulkan.device, attachment->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = psVulkanFindMemoryType(gameState, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX)
    {
        PS_LOG("Failed to find suitable memory type\n");
        return false;
    }

    result = vkAllocateMemory(gameState->vulkan.device, &allocInfo, NULL, &attachment->memory);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to allocate image memory\n");
        return false;
    }
    result = vkBindImageMemory(gameState->vulkan.device, attachment->image, attachment->memory, 0);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to bind image memory\n");
        return false;
    }

    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = attachment->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = psVulkanFramebufferAttachmentIsDepth(attachment) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    result = vkCreateImageView(gameState->vulkan.device, &viewInfo, NULL, &attachment->imageView);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create image view\n");
        return false;
    }

    attachment->format = format;
    attachment->subresourceRange.aspectMask = aspectMask;
    attachment->subresourceRange.baseArrayLayer = 0;
    attachment->subresourceRange.layerCount = 1;
    attachment->subresourceRange.baseMipLevel = 0;
    attachment->subresourceRange.levelCount = 1;

    attachment->attachmentDescription.flags = 0;
    attachment->attachmentDescription.format = format;
    attachment->attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment->attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment->attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment->attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment->attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment->attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (psVulkanFramebufferAttachmentIsDepthStencil(attachment))
    {
        attachment->attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    else
    {
        attachment->attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    return true;
}

static bool __psVulkanFramebufferCreateSampler(PS_GameState *gameState, PS_VulkanFramebuffer *framebuffer)
{
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(framebuffer != NULL);

    VkSamplerCreateInfo samplerInfo = {0};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    VkResult result = vkCreateSampler(gameState->vulkan.device, &samplerInfo, NULL, &framebuffer->sampler);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create framebuffer sampler\n");
        return false;
    }

    return true;
}

static bool __psVulkanFramebufferCreateRenderPassAndFramebuffer(PS_GameState *gameState, PS_VulkanFramebuffer *framebuffer)
{
    PS_ASSERT(gameState != NULL);
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
    {
        subpassDescription.pDepthStencilAttachment = &colorAttachmentReferences[1];
    }
    else
    {
        subpassDescription.pDepthStencilAttachment = NULL;
    }

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
    VkResult result = vkCreateRenderPass(gameState->vulkan.device, &renderPassInfo, NULL, &framebuffer->renderPass);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create render pass\n");
        return false;
    }

    static VkImageView attachmentViews[16] = {0};
    attachmentViews[0] = framebuffer->colorAttachment.imageView;
    if (framebuffer->hasDepthStencil)
    {
        attachmentViews[1] = framebuffer->depthStencilAttachment.imageView;
    }

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = framebuffer->renderPass;
    framebufferInfo.attachmentCount = attachmentCount;
    framebufferInfo.pAttachments = attachmentViews;
    framebufferInfo.width = framebuffer->width;
    framebufferInfo.height = framebuffer->height;
    framebufferInfo.layers = 1;
    result = vkCreateFramebuffer(gameState->vulkan.device, &framebufferInfo, NULL, &framebuffer->framebuffer);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create framebuffer\n");
        return false;
    }  


    return true;
}

bool psVulkanFramebufferAttachmentIsDepth(PS_VulkanFramebufferAttachment *attachment)
{
    PS_ASSERT(attachment != NULL);

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
        if (attachment->format == depthFormats[i])
        {
            isDepth = true;
            break;
        }
    }

    return isDepth;
}

bool psVulkanFramebufferAttachmentIsStencil(PS_VulkanFramebufferAttachment *attachment)
{
    PS_ASSERT(attachment != NULL);

    const VkFormat stencilFormats[] = {
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT};
    const size_t stencilFormatCount = sizeof(stencilFormats) / sizeof(stencilFormats[0]);

    bool isStencil = false;
    for (size_t i = 0; i < stencilFormatCount; ++i)
    {
        if (attachment->format == stencilFormats[i])
        {
            isStencil = true;
            break;
        }
    }

    return isStencil;
}

bool psVulkanFramebufferAttachmentIsDepthStencil(PS_VulkanFramebufferAttachment *attachment)
{
    PS_ASSERT(attachment != NULL);

    const bool isDepth = psVulkanFramebufferAttachmentIsDepth(attachment);
    const bool isStencil = psVulkanFramebufferAttachmentIsStencil(attachment);

    return isDepth || isStencil;
}

bool psVulkanFramebufferCreate(PS_GameState *gameState, PS_VulkanFramebuffer *framebuffer, int32_t width, int32_t height, bool hasDepthStencil, VkFormat colorFormat, VkFormat depthStencilFormat)
{
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(framebuffer != NULL);

    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->hasDepthStencil = hasDepthStencil;

    if (!__psVulkanFramebufferAttachmentCreate(gameState, &framebuffer->colorAttachment, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height))
    {
        PS_LOG("Failed to create color attachment\n");
        return false;
    }

    if (hasDepthStencil)
    {
        if (!__psVulkanFramebufferAttachmentCreate(gameState, &framebuffer->depthStencilAttachment, depthStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, width, height))
        {
            PS_LOG("Failed to create depth stencil attachment\n");
            return false;
        }
    }

    if (!__psVulkanFramebufferCreateSampler(gameState, framebuffer))
    {
        PS_LOG("Failed to create framebuffer sampler\n");
        return false;
    }

    // Create render pass and framebuffer here (omitted for brevity)
    if (!__psVulkanFramebufferCreateRenderPassAndFramebuffer(gameState, framebuffer))
    {
        PS_LOG("Failed to create render pass and framebuffer\n");
        return false;
    }

    return true;
}

void psVulkanFramebufferDestroy(PS_GameState *gameState, PS_VulkanFramebuffer *framebuffer)
{
    __psVulkanFramebufferAttachmentDestroy(gameState, &framebuffer->colorAttachment);
    if (framebuffer->hasDepthStencil)
    {
        __psVulkanFramebufferAttachmentDestroy(gameState, &framebuffer->depthStencilAttachment);
    }

    vkDestroySampler(gameState->vulkan.device, framebuffer->sampler, NULL);
    framebuffer->sampler = VK_NULL_HANDLE;

    vkDestroyRenderPass(gameState->vulkan.device, framebuffer->renderPass, NULL);
    framebuffer->renderPass = VK_NULL_HANDLE;

    vkDestroyFramebuffer(gameState->vulkan.device, framebuffer->framebuffer, NULL);
    framebuffer->framebuffer = VK_NULL_HANDLE;
}