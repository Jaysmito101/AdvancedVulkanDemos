#include "vulkan/avd_vulkan_image.h"
#include "avd_asset.h"
#include "core/avd_base.h"
#include "stb_image.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/avd_vulkan_framebuffer.h"
#include "vulkan/video/avd_vulkan_video_core.h"

bool avdVulkanFramebufferCreateSampler(
    AVD_Vulkan *vulkan,
    VkFilter filter,
    VkSamplerAddressMode addressMode,
    void *pNext,
    const char *label,
    VkSampler *outSampler)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(outSampler != NULL);

    VkSamplerCreateInfo samplerInfo = {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = filter,
        .minFilter               = filter,
        .addressModeU            = addressMode,
        .addressModeV            = addressMode,
        .addressModeW            = addressMode,
        .anisotropyEnable        = VK_FALSE,
        .maxAnisotropy           = 1.0f,
        .borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias              = 0.0f,
        .minLod                  = 0.0f,
        .maxLod                  = 1.0f,
        .pNext                   = pNext,
    };

    VkResult result = vkCreateSampler(vulkan->device, &samplerInfo, NULL, outSampler);
    AVD_CHECK_VK_RESULT(result, "Failed to create image sampler\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_SAMPLER,
        (uint64_t)*outSampler,
        "[Sampler][Core]:Vulkan/Sampler/%s",
        label ? label : "Unnamed");

    return true;
}

AVD_VulkanImageCreateInfo avdVulkanImageGetDefaultCreateInfo(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const char *label)
{
    AVD_VulkanImageCreateInfo info = {0};
    info.width                     = width;
    info.height                    = height;
    info.format                    = format;
    info.usage                     = usage;

    info.flags       = 0;
    info.depth       = 1;
    info.arrayLayers = 1;
    info.mipLevels   = 1;

    info.skipDefaultSubresourceCreation = false;

    info.samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.samplerFilter      = VK_FILTER_LINEAR;

    if (label) {
        snprintf(info.label, sizeof(info.label), "%s", label);
    } else {
        snprintf(info.label, sizeof(info.label), "Unnamed");
    }

    return info;
}

bool avdVulkanImageCreate(AVD_Vulkan *vulkan, AVD_VulkanImage *image, AVD_VulkanImageCreateInfo createInfo)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(image != NULL);
    AVD_ASSERT(!image->initialized);

    memset(image, 0, sizeof(AVD_VulkanImage));

    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.format            = createInfo.format;
    imageInfo.extent.width      = createInfo.width;
    imageInfo.extent.height     = createInfo.height;
    imageInfo.extent.depth      = createInfo.depth;
    imageInfo.mipLevels         = createInfo.mipLevels;
    imageInfo.arrayLayers       = createInfo.arrayLayers;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage             = createInfo.usage;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.flags             = createInfo.flags;

    bool isVideoDecodeImage = createInfo.usage & VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR ||
                              createInfo.usage & VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR ||
                              createInfo.usage & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
    bool isVideoEncodeImage = createInfo.usage & VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR ||
                              createInfo.usage & VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR ||
                              createInfo.usage & VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR;
    if (isVideoDecodeImage || isVideoEncodeImage) {
        imageInfo.pNext = avdVulkanVideoGetH264ProfileListInfo(isVideoDecodeImage);
    }

    VkResult result = vkCreateImage(vulkan->device, &imageInfo, NULL, &image->image);
    AVD_CHECK_VK_RESULT(result, "Failed to create image\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_IMAGE,
        (uint64_t)image->image,
        "[Image][Core]:Vulkan/Image/%s",
        createInfo.label[0] != '\0' ? createInfo.label : "Unnamed");

    VkMemoryRequirements memRequirements = {0};
    vkGetImageMemoryRequirements(vulkan->device, image->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = memRequirements.size;
    allocInfo.memoryTypeIndex      = avdVulkanFindMemoryType(vulkan, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    AVD_CHECK_MSG(allocInfo.memoryTypeIndex != UINT32_MAX, "Failed to find suitable memory type\n");

    result = vkAllocateMemory(vulkan->device, &allocInfo, NULL, &image->memory);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate image memory\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_DEVICE_MEMORY,
        (uint64_t)image->memory,
        "[Image/Memory][Core]:Vulkan/Image/%s/Memory",
        createInfo.label[0] != '\0' ? createInfo.label : "Unnamed");

    result = vkBindImageMemory(vulkan->device, image->image, image->memory, 0);
    AVD_CHECK_VK_RESULT(result, "Failed to bind image memory\n");

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_NONE;
    if (createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    } else if (createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        if (avdVulkanFormatIsDepth(createInfo.format)) {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (avdVulkanFormatIsStencil(createInfo.format)) {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        // fallback to color if no specific usage is set
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // store dimensions for upload
    image->info = createInfo;

    AVD_CHECK(avdVulkanFramebufferCreateSampler(
        vulkan,
        createInfo.samplerFilter,
        createInfo.samplerAddressMode,
        NULL,
        createInfo.label,
        &image->sampler));

    image->initialized = true;

    if (!createInfo.skipDefaultSubresourceCreation) {
        AVD_CHECK_MSG(
            avdVulkanImageSubresourceCreate(
                vulkan,
                image,
                (VkImageSubresourceRange){
                    .aspectMask     = aspectMask,
                    .baseMipLevel   = 0,
                    .levelCount     = createInfo.mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount     = createInfo.arrayLayers,
                },
                NULL,
                "Default",
                &image->defaultSubresource),
            "Failed to create default image subresource\n");
    }

    return true;
}

void avdVulkanImageDestroy(AVD_Vulkan *vulkan, AVD_VulkanImage *image)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(image != NULL);

    if (!image->initialized) {
        return;
    }

    if (!image->info.skipDefaultSubresourceCreation) {
        avdVulkanImageSubresourceDestroy(vulkan, &image->defaultSubresource);
    }

    vkDestroyImage(vulkan->device, image->image, NULL);
    vkDestroySampler(vulkan->device, image->sampler, NULL);
    vkFreeMemory(vulkan->device, image->memory, NULL);

    memset(image, 0, sizeof(AVD_VulkanImage));
}

bool avdVulkanImageSubresourceCreate(
    AVD_Vulkan *vulkan,
    AVD_VulkanImage *image,
    VkImageSubresourceRange subresourceRange,
    void *viewPNext,
    const char *label,
    AVD_VulkanImageSubresource *outSubresource)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(image != NULL);
    AVD_ASSERT(image->initialized);
    AVD_ASSERT(outSubresource != NULL);

    memset(outSubresource, 0, sizeof(AVD_VulkanImageSubresource));

    VkFormat format = image->info.format;
    if (subresourceRange.aspectMask & VK_IMAGE_ASPECT_PLANE_0_BIT_KHR) {
        AVD_CHECK(
            avdVulkanImageGetPlaneFormats(
                image->info.format,
                0,
                &format));
    } else if (subresourceRange.aspectMask & VK_IMAGE_ASPECT_PLANE_1_BIT_KHR) {
        AVD_CHECK(
            avdVulkanImageGetPlaneFormats(
                image->info.format,
                1,
                &format));
    } else if (subresourceRange.aspectMask & VK_IMAGE_ASPECT_PLANE_2_BIT_KHR) {
        AVD_CHECK(
            avdVulkanImageGetPlaneFormats(
                image->info.format,
                2,
                &format));
    }

    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                 = image->image;
    viewInfo.viewType              = subresourceRange.layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                = format;
    viewInfo.subresourceRange      = subresourceRange;
    viewInfo.components.r          = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g          = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b          = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a          = VK_COMPONENT_SWIZZLE_IDENTITY;

    VkImageViewUsageCreateInfo imageViewUsageInfo = {0};
    imageViewUsageInfo.sType                      = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
    bool isVideoDecodeImage                       = image->info.usage & VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR ||
                              image->info.usage & VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR ||
                              image->info.usage & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
    bool isVideoEncodeImage = image->info.usage & VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR ||
                              image->info.usage & VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR ||
                              image->info.usage & VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR;
    if (isVideoDecodeImage || isVideoEncodeImage) {
        imageViewUsageInfo.usage = image->info.usage;
        imageViewUsageInfo.pNext = viewPNext;
        viewInfo.pNext           = &imageViewUsageInfo;
    } else {
        viewInfo.pNext = viewPNext;
    }

    VkResult result = vkCreateImageView(vulkan->device, &viewInfo, NULL, &outSubresource->imageView);
    AVD_CHECK_VK_RESULT(result, "Failed to create image subresource view\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_IMAGE_VIEW,
        (uint64_t)outSubresource->imageView,
        "[ImageView][Core]:Vulkan/Image/%s/Subresource/%s",
        image->info.label[0] != '\0' ? image->info.label : "Unnamed",
        label ? label : "Unnamed");

    outSubresource->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    outSubresource->descriptorImageInfo.imageView   = outSubresource->imageView;
    outSubresource->descriptorImageInfo.sampler     = image->sampler;

    outSubresource->subresourceRange = subresourceRange;
    outSubresource->initialized      = true;

    return true;
}

void avdVulkanImageSubresourceDestroy(AVD_Vulkan *vulkan, AVD_VulkanImageSubresource *subresource)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(subresource != NULL);

    if (!subresource->initialized) {
        return;
    }

    vkDestroyImageView(vulkan->device, subresource->imageView, NULL);
    memset(subresource, 0, sizeof(AVD_VulkanImageSubresource));
}

bool avdVulkanImageTransitionLayout(AVD_VulkanImage *image, VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, AVD_VulkanImageSubresource *subresourceRange)
{
    AVD_ASSERT(image != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(image->initialized);

    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image->image,
        .subresourceRange    = subresourceRange ? subresourceRange->subresourceRange : image->defaultSubresource.subresourceRange,
    };

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldLayout) {
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
            // AVD_LOG_WARN("Unhandled old layout transition: %d", oldLayout);
            break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout) {
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
                if (srcStageMask != VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
                    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                }
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            // AVD_LOG_WARN("Unhandled new layout transition: %d", newLayout);
            break;
    }

    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][Core]:Vulkan/Image/TransitionLayout/%s/%d->%d", image->info.label, oldLayout, newLayout);

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, NULL,
        0, NULL,
        1, &barrier);

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);

    return true;
}

bool avdVulkanImageTransitionLayoutWithoutCommandBuffer(AVD_Vulkan *vulkan, AVD_VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, AVD_VulkanImageSubresource *subresourceRange)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(image != NULL);
    AVD_ASSERT(image->initialized);

    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = vulkan->graphicsCommandPool,
        .commandBufferCount = 1,
        .pNext              = NULL,
    };

    VkCommandBuffer commandBuffer;
    VkResult allocResult = vkAllocateCommandBuffers(vulkan->device, &allocInfo, &commandBuffer);
    AVD_CHECK_VK_RESULT(allocResult, "Failed to allocate command buffer for image layout transition");
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_COMMAND_BUFFER, commandBuffer, "[CommandBuffer][Core]:Vulkan/Image/TransitionLayout/%s", image->info.label);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pNext = NULL,
    };

    VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    AVD_CHECK_VK_RESULT(beginResult, "Failed to begin command buffer for image layout transition");

    bool transitionResult = avdVulkanImageTransitionLayout(image, commandBuffer, oldLayout, newLayout, srcStageMask, dstStageMask, subresourceRange);

    VkResult endResult = vkEndCommandBuffer(commandBuffer);
    AVD_CHECK_VK_RESULT(endResult, "Failed to end command buffer for image layout transition");
    AVD_CHECK_MSG(transitionResult, "Image layout transition command recording failed");

    VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &commandBuffer,
    };
    AVD_DEBUG_VK_QUEUE_BEGIN_LABEL(vulkan->graphicsQueue, NULL, "[Queue][Core]:Vulkan/Queue/ImageTransitionSubmit/%s", image->info.label);
    VkResult submitResult = vkQueueSubmit(vulkan->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    AVD_DEBUG_VK_QUEUE_END_LABEL(vulkan->graphicsQueue);
    AVD_CHECK_VK_RESULT(submitResult, "Failed to submit command buffer for image layout transition");

    AVD_DEBUG_VK_QUEUE_BEGIN_LABEL(vulkan->graphicsQueue, NULL, "[Queue][Core]:Vulkan/Queue/ImageTransitionWaitIdle/%s", image->info.label);
    VkResult waitResult = vkQueueWaitIdle(vulkan->graphicsQueue);
    AVD_DEBUG_VK_QUEUE_END_LABEL(vulkan->graphicsQueue);
    AVD_CHECK_VK_RESULT(waitResult, "Failed to wait for queue idle after image layout transition");

    vkFreeCommandBuffers(vulkan->device, vulkan->graphicsCommandPool, 1, &commandBuffer);
    return true;
}

// simple 2D image upload via staging buffer
bool avdVulkanImageUploadSimple(AVD_Vulkan *vulkan, AVD_VulkanImage *image, const void *srcData, AVD_VulkanImageSubresource *subresourceRange)
{
    AVD_ASSERT(vulkan && image && srcData);
    AVD_ASSERT(image->initialized);

    // infer bytes per pixel from format
    uint32_t bpp;
    switch (image->info.format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            bpp = 4;
            break;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R32G32B32_SFLOAT:
            bpp = 3;
            break;
        default:
            AVD_LOG_ERROR("Unsupported image format for upload: %d", image->info.format);
            return false;
    }
    VkDeviceSize imageSize = (VkDeviceSize)image->info.width * image->info.height * bpp;

    // create and fill staging buffer
    AVD_VulkanBuffer staging = {0};
    if (!avdVulkanBufferCreate(vulkan, &staging, imageSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Image/Upload/Staging")) {
        return false;
    }

    // since the staging buffer is host coherent, it will internally just map and memcpy
    if (!avdVulkanBufferUpload(vulkan, &staging, srcData, imageSize)) {
        avdVulkanBufferDestroy(vulkan, &staging);
        return false;
    }

    // allocate command buffer
    VkCommandBufferAllocateInfo bufAlloc = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = vulkan->graphicsCommandPool,
        .commandBufferCount = 1,
        .pNext              = NULL,
    };
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vulkan->device, &bufAlloc, &cmd);
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_COMMAND_BUFFER, cmd, "[CommandBuffer][Core]:Vulkan/Image/Upload/%s", image->info.label);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pNext = NULL,
    };
    vkBeginCommandBuffer(cmd, &beginInfo);

    // transition to transfer dst
    AVD_CHECK(avdVulkanImageTransitionLayout(image, cmd,
                                             VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                                             subresourceRange));

    VkImageSubresourceRange subresource = subresourceRange ? subresourceRange->subresourceRange : image->defaultSubresource.subresourceRange;

    // copy buffer to image
    VkBufferImageCopy region = {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = {
             .aspectMask     = subresource.aspectMask,
             .mipLevel       = subresource.baseMipLevel,
             .baseArrayLayer = subresource.baseArrayLayer,
             .layerCount     = subresource.layerCount,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            .width  = image->info.width,
            .height = image->info.height,
            .depth  = image->info.depth,
        },
    };

    AVD_DEBUG_VK_CMD_BEGIN_LABEL(cmd, NULL, "[Cmd][Core]:Vulkan/Image/CopyBufferToImage/%s", image->info.label);
    vkCmdCopyBufferToImage(cmd, staging.buffer, image->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    AVD_DEBUG_VK_CMD_END_LABEL(cmd);

    // transition to shader read
    AVD_CHECK(avdVulkanImageTransitionLayout(image, cmd,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                             NULL));

    vkEndCommandBuffer(cmd);

    // submit with temporary fence
    VkFenceCreateInfo fenceInfo = (VkFenceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = 0,
        .pNext = NULL,
    };
    VkFence fence;
    vkCreateFence(vulkan->device, &fenceInfo, NULL, &fence);
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_FENCE, fence, "[Fence][Core]:Vulkan/Image/UploadFence/%s", image->info.label);

    VkSubmitInfo submit = (VkSubmitInfo){
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
        .pNext              = NULL,
    };
    AVD_DEBUG_VK_QUEUE_BEGIN_LABEL(vulkan->graphicsQueue, NULL, "[Queue][Core]:Vulkan/Queue/ImageUploadSubmit/%s", image->info.label);
    vkQueueSubmit(vulkan->graphicsQueue, 1, &submit, fence);
    AVD_DEBUG_VK_QUEUE_END_LABEL(vulkan->graphicsQueue);

    vkWaitForFences(vulkan->device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(vulkan->device, fence, NULL);

    vkFreeCommandBuffers(vulkan->device, vulkan->graphicsCommandPool, 1, &cmd);
    avdVulkanBufferDestroy(vulkan, &staging);
    return true;
}

// load image file (8‑bit or HDR float), create Vulkan image and upload
bool avdVulkanImageLoadFromFile(AVD_Vulkan *vulkan, const char *filename, AVD_VulkanImage *image, const char *label)
{
    AVD_ASSERT(vulkan && filename && image);
    AVD_ASSERT(!image->initialized);

    int width, height, origChannels;
    void *pixels = NULL;
    bool isHDR   = stbi_is_hdr(filename);
    VkFormat format;

    // force load as RGBA (4 components)
    if (isHDR) {
        float *fdata = stbi_loadf(filename, &width, &height, &origChannels, 4);
        AVD_CHECK(fdata != NULL);
        format = VK_FORMAT_R32G32B32A32_SFLOAT;
        pixels = fdata;
    } else {
        unsigned char *cdata = stbi_load(filename, &width, &height, &origChannels, 4);
        AVD_CHECK(cdata != NULL);
        format = VK_FORMAT_R8G8B8A8_UNORM;
        pixels = cdata;
    }

    // create Vulkan image
    AVD_CHECK(avdVulkanImageCreate(
        vulkan, image,
        avdVulkanImageGetDefaultCreateInfo(
            (uint32_t)width, (uint32_t)height,
            format,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, label ? label : filename)));

    // upload pixel data
    AVD_CHECK(avdVulkanImageUploadSimple(vulkan, image, pixels, NULL));

    stbi_image_free(pixels);
    return true;
}

// load image from memory buffer, create Vulkan image and upload
bool avdVulkanImageLoadFromMemory(AVD_Vulkan *vulkan, const void *data, size_t dataSize, AVD_VulkanImage *image, const char *label)
{
    AVD_ASSERT(vulkan && data && image && dataSize > 0);
    AVD_ASSERT(!image->initialized);

    int width, height, origChannels;
    void *pixels = NULL;
    bool isHDR   = stbi_is_hdr_from_memory(data, (int)dataSize);
    VkFormat format;

    // force load as RGBA (4 components)
    if (isHDR) {
        float *fdata = stbi_loadf_from_memory(data, (int)dataSize, &width, &height, &origChannels, 4);
        AVD_CHECK(fdata != NULL);
        format = VK_FORMAT_R32G32B32A32_SFLOAT;
        pixels = fdata;
    } else {
        unsigned char *cdata = stbi_load_from_memory(data, (int)dataSize, &width, &height, &origChannels, 4);
        AVD_CHECK(cdata != NULL);
        format = VK_FORMAT_R8G8B8A8_UNORM;
        pixels = cdata;
    }

    // create Vulkan image
    AVD_CHECK(avdVulkanImageCreate(
        vulkan, image,
        avdVulkanImageGetDefaultCreateInfo(
            (uint32_t)width, (uint32_t)height,
            format,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, label)));

    // upload pixel data
    AVD_CHECK(avdVulkanImageUploadSimple(vulkan, image, pixels, NULL));

    stbi_image_free(pixels);
    return true;
}

bool avdVulkanImageLoadFromAsset(AVD_Vulkan *vulkan, const char *asset, AVD_VulkanImage *image, const char *label)
{
    AVD_ASSERT(vulkan && asset && image);
    AVD_ASSERT(!image->initialized);
    size_t assetSize         = 0;
    const uint8_t *assetData = avdAssetImage(asset, &assetSize);
    AVD_CHECK(assetData != NULL && assetSize > 0);
    return avdVulkanImageLoadFromMemory(vulkan, assetData, assetSize, image, label ? label : asset);
}

AVD_Bool avdVulkanImageIsFormatBiplanar(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
            return true;
        default:
            return false;
    }
}

AVD_Bool avdVulkanImageIsFormatTriplanar(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return true;
        default:
            return false;
    }
}

AVD_Bool avdVulkanImageGetPlaneFormats(VkFormat format, AVD_Size planeIndex, VkFormat *outFormatPlane0)
{
    if (outFormatPlane0 == NULL) {
        return false;
    }

    if (avdVulkanImageIsFormatBiplanar(format)) {
        switch (format) {
            case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
            case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
            case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
                if (planeIndex == 0) {
                    *outFormatPlane0 = VK_FORMAT_R8_UNORM;
                } else if (planeIndex == 1) {
                    *outFormatPlane0 = VK_FORMAT_R8G8_UNORM;
                } else {
                    return false;
                }
                return true;

            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
                if (planeIndex == 0) {
                    *outFormatPlane0 = VK_FORMAT_R10X6_UNORM_PACK16;
                } else if (planeIndex == 1) {
                    *outFormatPlane0 = VK_FORMAT_R10X6G10X6_UNORM_2PACK16;
                } else {
                    return false;
                }
                return true;

            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
                if (planeIndex == 0) {
                    *outFormatPlane0 = VK_FORMAT_R12X4_UNORM_PACK16;
                } else if (planeIndex == 1) {
                    *outFormatPlane0 = VK_FORMAT_R12X4G12X4_UNORM_2PACK16;
                } else {
                    return false;
                }
                return true;

            case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
            case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
            case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
                if (planeIndex == 0) {
                    *outFormatPlane0 = VK_FORMAT_R16_UNORM;
                } else if (planeIndex == 1) {
                    *outFormatPlane0 = VK_FORMAT_R16G16_UNORM;
                } else {
                    return false;
                }
                return true;

            default:
                return false;
        }
    } else if (avdVulkanImageIsFormatTriplanar(format)) {
        switch (format) {
            case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
            case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
            case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
                if (planeIndex <= 2) {
                    *outFormatPlane0 = VK_FORMAT_R8_UNORM;
                } else {
                    return false;
                }
                return true;

            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
                if (planeIndex <= 2) {
                    *outFormatPlane0 = VK_FORMAT_R10X6_UNORM_PACK16;
                } else {
                    return false;
                }
                return true;

            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
                if (planeIndex <= 2) {
                    *outFormatPlane0 = VK_FORMAT_R12X4_UNORM_PACK16;
                } else {
                    return false;
                }
                return true;

            case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
            case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
            case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
                if (planeIndex <= 2) {
                    *outFormatPlane0 = VK_FORMAT_R16_UNORM;
                } else {
                    return false;
                }
                return true;

            default:
                return false;
        }
    } else {
        if (planeIndex == 0) {
            *outFormatPlane0 = format;
            return true;
        }
        return false;
    }
}

bool avdVulkanImageYCbCrSubresourceCreate(
    AVD_Vulkan *vulkan,
    AVD_VulkanImage *image,
    VkImageSubresourceRange subresourceRange,
    bool useConversionIfAvailable,
    AVD_VulkanImageYCbCrSubresource *outSubresource)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(image != NULL);
    AVD_ASSERT(outSubresource != NULL);
    AVD_ASSERT(image->initialized);

    memset(outSubresource, 0, sizeof(AVD_VulkanImageYCbCrSubresource));

    AVD_CHECK_MSG(
        avdVulkanImageIsFormatBiplanar(image->info.format),
        "Image format is not a supported YCbCr biplanar format\n");

    outSubresource->usingConversionExtension = vulkan->supportedFeatures.ycbcrConversion && useConversionIfAvailable;
    outSubresource->subresourceRange         = subresourceRange;

    if (outSubresource->usingConversionExtension) {
        VkSamplerYcbcrConversionCreateInfo conversionCreateInfo = (VkSamplerYcbcrConversionCreateInfo){
            .sType      = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
            .format     = image->info.format,
            .ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601,
            .ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_NARROW,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .chromaFilter                = VK_FILTER_LINEAR,
            .forceExplicitReconstruction = VK_FALSE,
        };

        AVD_CHECK_VK_RESULT(
            vkCreateSamplerYcbcrConversion(
                vulkan->device,
                &conversionCreateInfo,
                NULL,
                &outSubresource->conv.conversion),
            "Failed to create YCbCr sampler conversion\n");
        AVD_DEBUG_VK_SET_OBJECT_NAME(
            VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION,
            outSubresource->conv.conversion,
            "[SamplerYcbcrConversion][Core]:Vulkan/Image/YCbCr/Conversion");

        VkSamplerYcbcrConversionInfo conversionInfo = (VkSamplerYcbcrConversionInfo){
            .sType      = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
            .conversion = outSubresource->conv.conversion,
        };

        AVD_CHECK(avdVulkanFramebufferCreateSampler(
            vulkan,
            VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            &conversionInfo,
            "Image/YCbCr/Sampler",
            &outSubresource->conv.sampler));

        AVD_CHECK(avdVulkanImageSubresourceCreate(
            vulkan,
            image,
            subresourceRange,
            &conversionInfo,
            "Image/YCbCr/Conversion",
            &outSubresource->conv.rgb));
        outSubresource->conv.rgb.descriptorImageInfo.sampler = outSubresource->conv.sampler;
    }

    // create raw subresources anyways
    {
        AVD_CHECK(avdVulkanImageSubresourceCreate(
            vulkan,
            image,
            (VkImageSubresourceRange){
                .aspectMask     = VK_IMAGE_ASPECT_PLANE_0_BIT,
                .baseMipLevel   = subresourceRange.baseMipLevel,
                .levelCount     = subresourceRange.levelCount,
                .baseArrayLayer = subresourceRange.baseArrayLayer,
                .layerCount     = subresourceRange.layerCount,
            },
            NULL,
            "Image/YCbCr/Luma",
            &outSubresource->raw.luma));

        AVD_CHECK(avdVulkanImageSubresourceCreate(
            vulkan,
            image,
            (VkImageSubresourceRange){
                .aspectMask     = VK_IMAGE_ASPECT_PLANE_1_BIT,
                .baseMipLevel   = subresourceRange.baseMipLevel,
                .levelCount     = subresourceRange.levelCount,
                .baseArrayLayer = subresourceRange.baseArrayLayer,
                .layerCount     = subresourceRange.layerCount,
            },
            NULL,
            "Image/YCbCr/Chroma",
            &outSubresource->raw.chroma));
    }

    outSubresource->initialized = true;

    return true;
}

void avdVulkanImageYCbCrSubresourceDestroy(AVD_Vulkan *vulkan, AVD_VulkanImageYCbCrSubresource *subresource)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(subresource != NULL);

    if (!subresource->initialized) {
        return;
    }

    if (subresource->conv.rgb.initialized) {
        avdVulkanImageSubresourceDestroy(vulkan, &subresource->conv.rgb);
        vkDestroySamplerYcbcrConversion(vulkan->device, subresource->conv.conversion, NULL);
        vkDestroySampler(vulkan->device, subresource->conv.sampler, NULL);
    }

    if (subresource->raw.chroma.initialized) {
        avdVulkanImageSubresourceDestroy(vulkan, &subresource->raw.chroma);
    }

    if (subresource->raw.luma.initialized) {
        avdVulkanImageSubresourceDestroy(vulkan, &subresource->raw.luma);
    }

    memset(subresource, 0, sizeof(AVD_VulkanImageYCbCrSubresource));
}