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

#endif // PS_VULKAN_H