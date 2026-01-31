#ifndef AVD_VULKAN_VIDEO_CORE_H
#define AVD_VULKAN_VIDEO_CORE_H

#include "core/avd_aligned_buffer.h"
#include "core/avd_types.h"
#include "vulkan/avd_vulkan_base.h"

#ifndef AVD_VULKAN_VIDEO_MAX_WIDTH
#define AVD_VULKAN_VIDEO_MAX_WIDTH 3840
#endif

#ifndef AVD_VULKAN_VIDEO_MAX_HEIGHT
#define AVD_VULKAN_VIDEO_MAX_HEIGHT 2160
#endif

// NOTE: these functions return pointers to static structures, do not free them
// and use them from a single thread only, therse are kind ahacky convienience functions
// not a great idea, but usually read-only access is needed
const VkVideoProfileInfoKHR* avdVulkanVideoGetH264DecodeProfileInfo();
const VkVideoProfileInfoKHR* avdVulkanVideoGetH264EncodeProfileInfo();
const VkVideoProfileInfoKHR* avdVulkanVideoGetH264ProfileInfo(AVD_Bool forDecode);

const VkVideoProfileListInfoKHR* avdVulkanVideoGetH264DecodeProfileListInfo();
const VkVideoProfileListInfoKHR* avdVulkanVideoGetH264EncodeProfileListInfo();
const VkVideoProfileListInfoKHR* avdVulkanVideoGetH264ProfileListInfo(AVD_Bool forDecode);


#endif // AVD_VULKAN_VIDEO_CORE_H
