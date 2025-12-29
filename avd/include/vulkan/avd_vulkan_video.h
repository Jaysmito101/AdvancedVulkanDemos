#ifndef AVD_VULKAN_VIDEO_H
#define AVD_VULKAN_VIDEO_H

#include "vulkan/avd_vulkan_base.h"

typedef struct {
    VkVideoSessionKHR session;
    VkVideoSessionParametersKHR sessionParameters;

    VkDeviceMemory memory;
} AVD_VulkanVideo;

bool avdVulkanVideoCreate(AVD_Vulkan *vulkan, AVD_VulkanVideo *video);



#endif // AVD_VULKAN_VIDEO_H