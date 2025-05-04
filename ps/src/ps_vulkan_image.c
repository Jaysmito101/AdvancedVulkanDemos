#include "ps_vulkan.h"
#include "ps_game_state.h"
#include <string.h>
#include "stb_image.h"

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
        // fallback to color if no specific usage is set
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

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

    // store dimensions for upload
    image->width  = width;
    image->height = height;

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

bool psVulkanImageTransitionLayout(PS_GameState *gameState, PS_VulkanImage *image, VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(image != NULL);
    PS_ASSERT(commandBuffer != VK_NULL_HANDLE);

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image->image;
    barrier.subresourceRange = image->subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            barrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            // PS_LOG("Unhandled old layout transition: %d\n", oldLayout);
            break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (barrier.srcAccessMask == 0) {
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            // PS_LOG("Unhandled new layout transition: %d\n", newLayout);
            break;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );

    return true;
}

// simple 2D image upload via staging buffer
bool psVulkanImageUploadSimple(PS_GameState *gameState, PS_VulkanImage *image, const void *srcData) {
    PS_ASSERT(gameState && image && srcData);

    // infer bytes per pixel from format
    uint32_t bpp;
    switch (image->format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            bpp = 4; break;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R32G32B32_SFLOAT:
            bpp = 3; break;
        default:
            PS_LOG("Unsupported image format for upload: %d\n", image->format);
            return false;
    }
    VkDeviceSize imageSize = (VkDeviceSize)image->width * image->height * bpp;

    // create and fill staging buffer
    PS_VulkanBuffer staging = {0};
    if (!psVulkanBufferCreate(gameState, &staging, imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return false;
    }
    void *mapped = NULL;
    if (!psVulkanBufferMap(gameState, &staging, &mapped)) {
        psVulkanBufferDestroy(gameState, &staging);
        return false;
    }
    memcpy(mapped, srcData, imageSize);
    psVulkanBufferUnmap(gameState, &staging);

    // allocate command buffer
    VkCommandBufferAllocateInfo bufAlloc = {0};
    bufAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufAlloc.commandPool = gameState->vulkan.graphicsCommandPool;
    bufAlloc.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(gameState->vulkan.device, &bufAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // transition to transfer dst
    psVulkanImageTransitionLayout(gameState, image, cmd,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // copy buffer to image
    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = image->subresourceRange.aspectMask;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = image->width;
    region.imageExtent.height = image->height;
    region.imageExtent.depth = 1;
    region.imageOffset = (VkOffset3D){0,0,0};
    region.imageExtent = (VkExtent3D){image->width, image->height, 1};
    vkCmdCopyBufferToImage(cmd, staging.buffer, image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // transition to shader read
    psVulkanImageTransitionLayout(gameState, image, cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    vkEndCommandBuffer(cmd);

    // submit with temporary fence
    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    vkCreateFence(gameState->vulkan.device, &fenceInfo, NULL, &fence);

    VkSubmitInfo submit = {0};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(gameState->vulkan.graphicsQueue, 1, &submit, fence);

    vkWaitForFences(gameState->vulkan.device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(gameState->vulkan.device, fence, NULL);

    vkFreeCommandBuffers(gameState->vulkan.device, gameState->vulkan.graphicsCommandPool, 1, &cmd);
    psVulkanBufferDestroy(gameState, &staging);
    return true;
}

// load image file (8‑bit or HDR float), create Vulkan image and upload
bool psVulkanImageLoadFromFile(PS_GameState *gameState, const char *filename, PS_VulkanImage *image) {
    PS_ASSERT(gameState && filename && image);

    int width, height, origChannels;
    void *pixels = NULL;
    bool isHDR = stbi_is_hdr(filename);
    VkFormat format;

    // force load as RGBA (4 components)
    if (isHDR) {
        float *fdata = stbi_loadf(filename, &width, &height, &origChannels, 4);
        if (!fdata) {
            PS_LOG("Failed to load HDR image: %s\n", filename);
            return false;
        }
        format = VK_FORMAT_R32G32B32A32_SFLOAT;
        pixels = fdata;
    } else {
        unsigned char *cdata = stbi_load(filename, &width, &height, &origChannels, 4);
        if (!cdata) {
            PS_LOG("Failed to load image: %s\n", filename);
            return false;
        }
        format = VK_FORMAT_R8G8B8A8_UNORM;
        pixels = cdata;
    }

    // create Vulkan image
    if (!psVulkanImageCreate(
            gameState, image, format,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            (uint32_t)width, (uint32_t)height)) {
        PS_LOG("Failed to create Vulkan image for file: %s\n", filename);
        stbi_image_free(pixels);
        return false;
    }

    // upload pixel data
    if (!psVulkanImageUploadSimple(gameState, image, pixels)) {
        PS_LOG("Failed to upload image data: %s\n", filename);
        stbi_image_free(pixels);
        return false;
    }

    stbi_image_free(pixels);
    return true;
}