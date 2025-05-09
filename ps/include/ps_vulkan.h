#ifndef PS_VULKAN_H
#define PS_VULKAN_H

#define API_VERSION VK_API_VERSION_1_2
#include <vulkan/vulkan.h>

#include "ps_common.h"

bool psVulkanInit(PS_GameState *gameState); 
void psVulkanShutdown(PS_GameState *gameState);
uint32_t psVulkanFindMemoryType(PS_GameState *gameState, uint32_t typeFilter, VkMemoryPropertyFlags properties);

bool psVulkanSwapchainCreate(PS_GameState *gameState);
bool psVulkanSwapchainRecreate(PS_GameState *gameState);
void psVulkanSwapchainDestroy(PS_GameState *gameState);
VkResult psVulkanSwapchainAcquireNextImage(PS_GameState *gameState, uint32_t *imageIndex, VkSemaphore semaphore);
VkResult psVulkanSwapchainPresent(PS_GameState *gameState, uint32_t imageIndex, VkSemaphore waitSemaphore);

bool psVulkanRendererCreate(PS_GameState *gameState);
void psVulkanRendererDestroy(PS_GameState *gameState);
void psVulkanRendererRender(PS_GameState *gameState);

bool psVulkanPresentationInit(PS_GameState *gameState);
void psVulkanPresentationDestroy(PS_GameState *gameState); 
bool psVulkanPresentationRender(PS_GameState *gameState, uint32_t imageIndex);

bool psVulkanSceneInit(PS_GameState *gameState);
void psVulkanSceneDestroy(PS_GameState *gameState); 
bool psVulkanSceneRender(PS_GameState *gameState, uint32_t imageIndex);

bool psVulkanFormatIsDepth(VkFormat format);
bool psVulkanFormatIsStencil(VkFormat format);
bool psVulkanFormatIsDepthStencil(VkFormat format);
bool psVulkanFramebufferCreate(PS_GameState *gameState, PS_VulkanFramebuffer *framebuffer, int32_t width, int32_t height, bool hasDepthStencil, VkFormat colorFormat, VkFormat depthStencilFormat);
void psVulkanFramebufferDestroy(PS_GameState *gameState, PS_VulkanFramebuffer *framebuffer);

bool psVulkanBufferCreate(PS_GameState *gameState, PS_VulkanBuffer *buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
void psVulkanBufferDestroy(PS_GameState *gameState, PS_VulkanBuffer *buffer);

bool psVulkanImageCreate(PS_GameState *gameState, PS_VulkanImage *image, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height);
bool psVulkanImageTransitionLayout(PS_GameState *gameState, PS_VulkanImage *image, VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
bool psVulkanImageTransitionLayoutWithoutCommandBuffer(PS_GameState *gameState, PS_VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
void psVulkanImageDestroy(PS_GameState *gameState, PS_VulkanImage *image);
bool psVulkanBufferMap(PS_GameState *gameState, PS_VulkanBuffer *buffer, void **data);
void psVulkanBufferUnmap(PS_GameState *gameState, PS_VulkanBuffer *buffer);
bool psVulkanBufferUpload(PS_GameState *gameState, PS_VulkanBuffer *buffer, const void *srcData, VkDeviceSize size);
bool psVulkanImageUploadSimple(PS_GameState *gameState, PS_VulkanImage *image, const void *srcData);
bool psVulkanImageLoadFromFile(PS_GameState *gameState, const char *filename, PS_VulkanImage *image);
bool psVulkanImageLoadFromMemory(PS_GameState *gameState, const void *data, size_t dataSize, PS_VulkanImage *image);

#endif // PS_VULKAN_H