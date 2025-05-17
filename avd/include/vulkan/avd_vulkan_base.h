#ifndef AVD_VULKAN_CORE_H
#define AVD_VULKAN_CORE_H

#include "core/avd_core.h"

// third party includes
#include "glfw/glfw3.h"

#define API_VERSION VK_API_VERSION_1_2
#include "volk.h"

typedef struct AVD_Vulkan {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;
    VkDescriptorPool descriptorPool;
    
    int32_t graphicsQueueFamilyIndex;
    int32_t computeQueueFamilyIndex;

#ifdef AVD_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    bool debugLayersEnabled;
#endif
} AVD_Vulkan;
 
bool avdVulkanInit(AVD_Vulkan *vulkan, AVD_Window *window, VkSurfaceKHR* surface);
void avdVulkanShutdown(AVD_Vulkan *vulkan);
void avdVulkanWaitIdle(AVD_Vulkan *vulkan);
void avdVulkanDestroySurface(AVD_Vulkan *vulkan, VkSurfaceKHR surface);

uint32_t avdVulkanFindMemoryType(AVD_Vulkan* vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties);


#endif // AVD_VULKAN_CORE_H