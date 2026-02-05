#include "vulkan/video/avd_vulkan_video_dpb.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include <string.h>

// NOTE: Ideally we should call vkGetPhysicalDeviceVideoFormatPropertiesKHR witht he profile and image usage info
// to check which format is supported, but for now we just assume this one is supported everywhere.... :)
static const VkFormat __AVD_VULKAN_VIDEO_DPB_FORMAT = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;

static bool __avdVulkanVideoDPBLoadSystemFlags(AVD_Vulkan *vulkan, AVD_VulkanVideoDPB *dpb)
{

    dpb->decodeOutputCoincideSupported = (vulkan->supportedFeatures.videoDecodeCapabilities.flags &
                                          VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_COINCIDE_BIT_KHR) != 0;
    dpb->decodeOutputDistinctSupported = (vulkan->supportedFeatures.videoDecodeCapabilities.flags &
                                          VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_DISTINCT_BIT_KHR) != 0;

    if (!dpb->decodeOutputCoincideSupported && !dpb->decodeOutputDistinctSupported) {
        AVD_LOG_ERROR("Neither DPB and output coincide nor DPB and output distinct are supported for video DPB %s", dpb->label);
        return false;
    }

    return true;
}

static bool __avdVulkanVideoDPBCreateFreeResources(AVD_Vulkan *vulkan, AVD_VulkanVideoDPB *dpb)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(dpb != NULL);

    if (dpb->dpb.image != VK_NULL_HANDLE) {
        avdVulkanImageDestroy(vulkan, &dpb->dpb);
    }

    if (dpb->decodedOutputImage.image != VK_NULL_HANDLE) {
        avdVulkanImageDestroy(vulkan, &dpb->decodedOutputImage);
    }

    return true;
}

static bool __avdVulkanVideoDPBCreateImages(AVD_Vulkan *vulkan, AVD_VulkanVideoDPB *dpb, bool forDecode, AVD_UInt32 width, AVD_UInt32 height, AVD_UInt32 numDPBSlots)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(dpb != NULL);
    AVD_ASSERT(numDPBSlots < vulkan->supportedFeatures.videoCapabilitiesDecode.maxDpbSlots);

    dpb->numDPBSlots = numDPBSlots;

    AVD_VulkanImageCreateInfo dpbImageInfo = (AVD_VulkanImageCreateInfo){
        .width       = width,
        .height      = height,
        .format      = __AVD_VULKAN_VIDEO_DPB_FORMAT,
        .usage       = forDecode ? VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR : VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR,
        .depth       = 1,
        .arrayLayers = (AVD_UInt32)dpb->numDPBSlots,
        .mipLevels   = 1,
        .flags       = 0,
    };
    if (dpb->decodeOutputCoincideSupported) {
        if (forDecode) {
            dpbImageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
            dpbImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        } else {
            dpbImageInfo.usage |= VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR;
            dpbImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
    }
    AVD_CHECK_MSG(
        avdVulkanImageCreate(vulkan, &dpb->dpb, dpbImageInfo),
        "Failed to create DPB image for video DPB %s",
        dpb->label);

    if (!dpb->decodeOutputCoincideSupported) {
        AVD_VulkanImageCreateInfo decodedOutputImageInfo = (AVD_VulkanImageCreateInfo){
            .width       = width,
            .height      = height,
            .format      = __AVD_VULKAN_VIDEO_DPB_FORMAT,
            .usage       = forDecode ? VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                                     : VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT, // encode doesnt make any sense here though :)
            .depth       = 1,
            .arrayLayers = 2,
            .mipLevels   = 1,
            .flags       = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        };

        AVD_CHECK_MSG(
            avdVulkanImageCreate(vulkan, &dpb->decodedOutputImage, decodedOutputImageInfo),
            "Failed to create decoded output image for video DPB %s",
            dpb->label);
    }
    return true;
}

bool avdVulkanVideoDecodeDPBCreate(
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDPB *dpb,
    AVD_UInt32 width,
    AVD_UInt32 height,
    AVD_UInt32 numDPBSlots,
    const char *label)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(dpb != NULL);
    AVD_ASSERT(width <= AVD_VULKAN_VIDEO_MAX_WIDTH);
    AVD_ASSERT(height <= AVD_VULKAN_VIDEO_MAX_HEIGHT);

    memset(dpb, 0, sizeof(AVD_VulkanVideoDPB));

    strncpy(dpb->label, label, sizeof(dpb->label) - 1);
    dpb->label[sizeof(dpb->label) - 1] = '\0';

    dpb->width  = width;
    dpb->height = height;
    dpb->format = __AVD_VULKAN_VIDEO_DPB_FORMAT;

    AVD_CHECK_MSG(
        __avdVulkanVideoDPBLoadSystemFlags(vulkan, dpb),
        "Failed to load system flags for video DPB %s",
        dpb->label);

    AVD_CHECK_MSG(
        __avdVulkanVideoDPBCreateImages(vulkan, dpb, true, width, height, numDPBSlots),
        "Failed to create images for video DPB %s",
        dpb->label);

    for (AVD_Size i = 0; i < dpb->numDPBSlots; ++i) {
        dpb->imageLayouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    dpb->initialized = true;

    return true;
}

void avdVulkanVideoDPBDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDPB *dpb)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(dpb != NULL);

    if (!dpb->initialized) {
        return;
    }

    if (!__avdVulkanVideoDPBCreateFreeResources(vulkan, dpb)) {
        AVD_LOG_ERROR("Failed to free video DPB resources");
    }

    memset(dpb, 0, sizeof(AVD_VulkanVideoDPB));
}

bool avdVulkanVideoDPBInitializeLayouts(AVD_Vulkan *vulkan, AVD_VulkanVideoDPB *dpb, VkCommandBuffer commandBuffer)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(dpb != NULL);

    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = dpb->dpb.image,
        .srcAccessMask       = VK_ACCESS_NONE,
        .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        .subresourceRange    = dpb->dpb.defaultSubresource.subresourceRange,
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrier);

    for (AVD_Size i = 0; i < dpb->numDPBSlots; ++i) {
        dpb->imageLayouts[i] = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR;
    }

    dpb->layoutInitialized = true;

    return true;
}

bool avdVulkanVideoDecodeDPBTransitionImageLayout(
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDPB *dpb,
    AVD_Size slotIndex,
    VkImageLayout newLayout,
    VkCommandBuffer commandBuffer)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(dpb != NULL);
    AVD_ASSERT(slotIndex < dpb->numDPBSlots);
    AVD_ASSERT(dpb->initialized);

    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = dpb->imageLayouts[slotIndex],
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = dpb->dpb.image,
        .srcAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
        .subresourceRange    = {
               .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
               .baseMipLevel   = 0,
               .levelCount     = VK_REMAINING_MIP_LEVELS,
               .baseArrayLayer = slotIndex,
               .layerCount     = 1,
        },
    };
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrier);

    dpb->imageLayouts[slotIndex] = newLayout;

    return true;
}

bool avdVulkanVideoDecodeDPBOutputImageTransitionImageLayout(
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDPB *dpb,
    VkImageLayout newLayout,
    VkCommandBuffer commandBuffer)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(dpb != NULL);
    AVD_ASSERT(dpb->initialized);

    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = dpb->decodedOutputImage.image,
        .srcAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
        .subresourceRange    = dpb->decodedOutputImage.defaultSubresource.subresourceRange,
    };
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrier);

    return true;
}