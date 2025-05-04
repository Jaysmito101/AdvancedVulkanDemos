#include "ps_vulkan.h"

static bool __psVulkanFramebufferCreateSampler(PS_GameState *gameState, PS_VulkanImage *image)
{
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(image != NULL);

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

    VkResult result = vkCreateSampler(gameState->vulkan.device, &samplerInfo, NULL, &image->sampler);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create image sampler\n");
        return false;
    }

    return true;
}

bool psVulkanImageCreate(PS_GameState *gameState, PS_VulkanImage *image, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height)
{
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(image != NULL);

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_NONE;
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        if (psVulkanFormatIsDepth(format))
        {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (psVulkanFormatIsStencil(format))
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

    VkResult result = vkCreateImage(gameState->vulkan.device, &imageInfo, NULL, &image->image);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create image\n");
        return false;
    }

    
    VkMemoryRequirements memRequirements = {0};
    vkGetImageMemoryRequirements(gameState->vulkan.device, image->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = psVulkanFindMemoryType(gameState, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX)
    {
        PS_LOG("Failed to find suitable memory type\n");
        return false;
    }

    result = vkAllocateMemory(gameState->vulkan.device, &allocInfo, NULL, &image->memory);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to allocate image memory\n");
        return false;
    }
    result = vkBindImageMemory(gameState->vulkan.device, image->image, image->memory, 0);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to bind image memory\n");
        return false;
    }

    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = psVulkanFormatIsDepth(image->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    result = vkCreateImageView(gameState->vulkan.device, &viewInfo, NULL, &image->imageView);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create image view\n");
        return false;
    }

    image->format = format;
    
    image->subresourceRange.aspectMask = aspectMask;
    image->subresourceRange.baseArrayLayer = 0;
    image->subresourceRange.layerCount = 1;
    image->subresourceRange.baseMipLevel = 0;
    image->subresourceRange.levelCount = 1;
    
    if (!__psVulkanFramebufferCreateSampler(gameState,  image))
    {
        PS_LOG("Failed to create framebuffer sampler\n");
        return false;
    }


    image->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image->descriptorImageInfo.imageView = image->imageView;
    image->descriptorImageInfo.sampler = image->sampler;

    return true;
}

void psVulkanImageDestroy(PS_GameState *gameState, PS_VulkanImage *image)
{
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(image != NULL);

    vkDestroyImageView(gameState->vulkan.device, image->imageView, NULL);
    image->imageView = VK_NULL_HANDLE;

    vkDestroyImage(gameState->vulkan.device, image->image, NULL);
    image->image = VK_NULL_HANDLE;
    
    vkDestroySampler(gameState->vulkan.device, image->sampler, NULL);
    image->sampler = VK_NULL_HANDLE;

    vkFreeMemory(gameState->vulkan.device, image->memory, NULL);
    image->memory = VK_NULL_HANDLE;
}