#ifndef PS_VULKAN_H
#define PS_VULKAN_H


#include "ps_common.h"
#include "ps_pipeline_utils.h"

bool psVulkanInit(PS_Vulkan *vulkan, PS_Window *window);
void psVulkanShutdown(PS_Vulkan *vulkan);
uint32_t psVulkanFindMemoryType(PS_Vulkan* vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties);

bool psVulkanSwapchainCreate(PS_Vulkan *vulkan, PS_Window* window);
bool psVulkanSwapchainRecreate(PS_Vulkan *vulkan, PS_Window* window);
void psVulkanSwapchainDestroy(PS_Vulkan *vulkan);
VkResult psVulkanSwapchainAcquireNextImage(PS_Vulkan *vulkan, uint32_t *imageIndex, VkSemaphore semaphore);
VkResult psVulkanSwapchainPresent(PS_Vulkan *vulkan, uint32_t imageIndex, VkSemaphore waitSemaphore);

bool psVulkanRendererCreate(PS_Vulkan *vulkan);
void psVulkanRendererDestroy(PS_Vulkan *vulkan);
void psVulkanRendererRender(PS_GameState *gameState);

bool psVulkanPresentationInit(PS_Vulkan *gameState);
void psVulkanPresentationDestroy(PS_Vulkan *gameState); 
bool psVulkanPresentationRender(PS_Vulkan *gameState, uint32_t imageIndex);

bool psVulkanSceneInit(PS_GameState *gameState);
void psVulkanSceneDestroy(PS_GameState *gameState); 
bool psVulkanSceneRender(PS_GameState *gameState, uint32_t imageIndex);

bool psVulkanFormatIsDepth(VkFormat format);
bool psVulkanFormatIsStencil(VkFormat format);
bool psVulkanFormatIsDepthStencil(VkFormat format);
bool psVulkanFramebufferCreate(PS_Vulkan *vulkan, PS_VulkanFramebuffer *framebuffer, int32_t width, int32_t height, bool hasDepthStencil, VkFormat colorFormat, VkFormat depthStencilFormat);
void psVulkanFramebufferDestroy(PS_Vulkan *vulkan, PS_VulkanFramebuffer *framebuffer);

bool psVulkanBufferCreate(PS_Vulkan* vulkan, PS_VulkanBuffer *buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
void psVulkanBufferDestroy(PS_Vulkan* vulkan, PS_VulkanBuffer *buffer);

bool psVulkanImageCreate(PS_Vulkan* vulkan, PS_VulkanImage *image, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height);
bool psVulkanImageTransitionLayout(PS_VulkanImage *image, VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
bool psVulkanImageTransitionLayoutWithoutCommandBuffer(PS_Vulkan* vulkan, PS_VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
void psVulkanImageDestroy(PS_Vulkan* vulkan, PS_VulkanImage *image);
bool psVulkanBufferMap(PS_Vulkan* vulkan, PS_VulkanBuffer *buffer, void **data);
void psVulkanBufferUnmap(PS_Vulkan* vulkan, PS_VulkanBuffer *buffer);
bool psVulkanBufferUpload(PS_Vulkan* vulkan, PS_VulkanBuffer *buffer, const void *srcData, VkDeviceSize size);
bool psVulkanImageUploadSimple(PS_Vulkan* vulkan, PS_VulkanImage *image, const void *srcData);
bool psVulkanImageLoadFromFile(PS_Vulkan* vulkan, const char *filename, PS_VulkanImage *image);
bool psVulkanImageLoadFromMemory(PS_Vulkan* vulkan, const void *data, size_t dataSize, PS_VulkanImage *image);
bool psVulkanImageLoadFromAsset(PS_Vulkan* vulkan, const char *asset, PS_VulkanImage *image);

#endif // PS_VULKAN_H