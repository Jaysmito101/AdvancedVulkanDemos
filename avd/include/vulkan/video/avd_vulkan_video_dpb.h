#ifndef AVD_VULKAN_VIDEO_DPB_H
#define AVD_VULKAN_VIDEO_DPB_H

#include "vulkan/avd_vulkan_image.h"
#include "vulkan/video/avd_vulkan_video_core.h"

#define AVD_VULKAN_VIDEO_DPB_MAX_SLOTS 16

typedef struct {
    AVD_Bool initialized;

    AVD_UInt32 width;
    AVD_UInt32 height;

    AVD_VulkanImage dpb;
    AVD_VulkanImage decodedOutputImage;

    // layout for each DPB slot + decoded output image
    VkImageLayout imageLayouts[AVD_VULKAN_VIDEO_DPB_MAX_SLOTS + 1];

    AVD_Size numDPBSlots;

    AVD_Bool decodeOutputCoincideSupported;
    AVD_Bool decodeOutputDistinctSupported;

    char label[64];
} AVD_VulkanVideoDPB;

bool avdVulkanVideoDecodeDPBCreate(
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDPB *dpb,
    AVD_UInt32 width,
    AVD_UInt32 height,
    const char *label);
void avdVulkanVideoDPBDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDPB *dpb);

#endif // AVD_VULKAN_VIDEO_DPB_H