#ifndef AVD_VULKAN_RENDERER_H
#define AVD_VULKAN_RENDERER_H

#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_framebuffer.h"
#include "vulkan/avd_vulkan_swapchain.h"
#include "vulkan/vulkan_core.h"

#ifndef AVD_MAX_IN_FLIGHT_FRAMES
#define AVD_MAX_IN_FLIGHT_FRAMES 16
#endif

typedef struct AVD_VulkanRendererResources {
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence renderFence;
} AVD_VulkanRendererResources;

typedef struct AVD_VulkanRenderer {
    AVD_VulkanRendererResources resources[AVD_MAX_IN_FLIGHT_FRAMES];
    uint32_t numInFlightFrames;
    uint32_t currentFrameIndex;
    uint32_t currentImageIndex;

    uint32_t width;
    uint32_t height;

    // AVD_VulkanPresentation presentation;

    AVD_VulkanFramebuffer sceneFramebuffer;
} AVD_VulkanRenderer;

bool avdVulkanRendererCreate(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain, uint32_t width, uint32_t height);
void avdVulkanRendererDestroy(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan);
bool avdVulkanRendererRecreateResources(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain);
bool avdVulkanRendererBegin(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain);
bool avdVulkanRendererEnd(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain);
bool avdVulkanRendererCancelFrame(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan);
VkCommandBuffer avdVulkanRendererGetCurrentCmdBuffer(AVD_VulkanRenderer* renderer);

#endif // AVD_VULKAN_RENDERER_H
