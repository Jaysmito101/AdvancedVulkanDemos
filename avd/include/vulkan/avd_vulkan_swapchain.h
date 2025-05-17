#ifndef AVD_VULKAN_SWAPCHAIN_H
#define AVD_VULKAN_SWAPCHAIN_H

#include "vulkan/avd_vulkan_base.h"

typedef struct AVD_VulkanSwapchain {
    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;
    VkImage images[64];
    VkImageView imageViews[64];
    VkFramebuffer framebuffers[64];
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    uint32_t imageCount;
    VkExtent2D extent;
    VkRenderPass renderPass;
    bool swapchainRecreateRequired;
    bool swapchainReady;
} AVD_VulkanSwapchain;


bool avdVulkanSwapchainCreate(AVD_VulkanSwapchain* swapchain, AVD_Vulkan *vulkan, VkSurfaceKHR surface, AVD_Window* window);
bool avdVulkanSwapchainRecreate(AVD_VulkanSwapchain* swapchain, AVD_Vulkan *vulkan, AVD_Window* window);
void avdVulkanSwapchainDestroy(AVD_VulkanSwapchain* swapchain, AVD_Vulkan *vulkan);
VkResult avdVulkanSwapchainAcquireNextImage(AVD_VulkanSwapchain* swapchain, AVD_Vulkan *vulkan, uint32_t *imageIndex, VkSemaphore semaphore, VkFence fence);
VkResult avdVulkanSwapchainPresent(AVD_VulkanSwapchain* swapchain, AVD_Vulkan *vulkan, uint32_t imageIndex, VkSemaphore waitSemaphore);
bool avdVulkanSwapchainRecreateIfNeeded(AVD_VulkanSwapchain* swapchain, AVD_Vulkan* vulkan, AVD_Window* window);

#endif // AVD_VULKAN_SWAPCHAIN_H