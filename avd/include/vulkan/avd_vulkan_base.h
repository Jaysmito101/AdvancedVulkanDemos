#ifndef AVD_VULKAN_CORE_H
#define AVD_VULKAN_CORE_H

#define API_VERSION VK_API_VERSION_1_4
#include "volk.h"

#include "core/avd_core.h"


// third party includes
//
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"


#ifndef AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE
#define AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE 1024
#endif

typedef enum {
    AVD_VULKAN_DESCRIPTOR_TYPE_SAMPLER = 0,
    AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    AVD_VULKAN_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    AVD_VULKAN_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    AVD_VULKAN_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    AVD_VULKAN_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    AVD_VULKAN_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    AVD_VULKAN_DESCRIPTOR_TYPE_COUNT
} AVD_VulkanDescriptorType;

typedef struct AVD_Vulkan {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;
    VkDescriptorPool descriptorPool;
    VkDescriptorPool bindlessDescriptorPool;

    VkDescriptorSetLayout bindlessDescriptorSetLayout;
    VkDescriptorSet bindlessDescriptorSet;

    int32_t graphicsQueueFamilyIndex;
    int32_t computeQueueFamilyIndex;

#ifdef AVD_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    bool debugLayersEnabled;
#endif
} AVD_Vulkan;

bool avdVulkanInit(AVD_Vulkan *vulkan, AVD_Window *window, VkSurfaceKHR *surface);
void avdVulkanShutdown(AVD_Vulkan *vulkan);
void avdVulkanWaitIdle(AVD_Vulkan *vulkan);
void avdVulkanDestroySurface(AVD_Vulkan *vulkan, VkSurfaceKHR surface);

uint32_t avdVulkanFindMemoryType(AVD_Vulkan *vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkDescriptorType avdVulkanToVkDescriptorType(AVD_VulkanDescriptorType type);

#endif // AVD_VULKAN_CORE_H
