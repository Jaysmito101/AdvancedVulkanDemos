#ifndef PS_VULKAN_H
#define PS_VULKAN_H

#define API_VERSION VK_API_VERSION_1_2
#include <vulkan/vulkan.h>

#include "ps_common.h"

bool psVulkanInit(PS_GameState *gameState); 
void psVulkanShutdown(PS_GameState *gameState);

bool psVulkanSwapchainCreate(PS_GameState *gameState);
bool psVulkanSwapchainRecreate(PS_GameState *gameState);
void psVulkanSwapchainDestroy(PS_GameState *gameState);
VkResult psVulkanSwapchainAcquireNextImage(PS_GameState *gameState, uint32_t *imageIndex, VkSemaphore semaphore);
VkResult psVulkanSwapchainPresent(PS_GameState *gameState, uint32_t imageIndex, VkSemaphore waitSemaphore);

bool psVulkanRendererCreate(PS_GameState *gameState);
void psVulkanRendererDestroy(PS_GameState *gameState);
void psVulkanRendererRender(PS_GameState *gameState);
bool psVulkanRendererRenderPresentationPass(PS_GameState *gameState, uint32_t imageIndex);


#endif // PS_VULKAN_H