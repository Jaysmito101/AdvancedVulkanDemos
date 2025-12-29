#ifndef AVD_VULKAN_VIDEO_H
#define AVD_VULKAN_VIDEO_H

#include "math/avd_math_base.h"
#include "vulkan/avd_vulkan_base.h"

#ifndef AVD_VULKAN_VIDEO_MAX_WIDTH
#define AVD_VULKAN_VIDEO_MAX_WIDTH 3840
#endif

#ifndef AVD_VULKAN_VIDEO_MAX_HEIGHT
#define AVD_VULKAN_VIDEO_MAX_HEIGHT 2160
#endif

typedef struct {
    VkVideoSessionKHR session;
    VkVideoSessionParametersKHR sessionParameters;

    VkDeviceMemory memory[128];
    AVD_UInt32 memoryAllocationCount;
} AVD_VulkanVideo;

bool avdVulkanVideoCreate(AVD_Vulkan *vulkan, AVD_VulkanVideo *video);
void avdVulkanVideoDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideo *video);


#endif // AVD_VULKAN_VIDEO_H