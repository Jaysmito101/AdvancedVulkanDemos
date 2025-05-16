#ifndef AVD_VULKAN_H
#define AVD_VULKAN_H


#include "avd_common.h"
#include "avd_pipeline_utils.h"

bool avdVulkanInit(AVD_Vulkan *vulkan, AVD_Window *window);
void avdVulkanShutdown(AVD_Vulkan *vulkan);
uint32_t avdVulkanFindMemoryType(AVD_Vulkan* vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties);

bool avdVulkanSwapchainCreate(AVD_Vulkan *vulkan, AVD_Window* window);
bool avdVulkanSwapchainRecreate(AVD_Vulkan *vulkan, AVD_Window* window);
void avdVulkanSwapchainDestroy(AVD_Vulkan *vulkan);
VkResult avdVulkanSwapchainAcquireNextImage(AVD_Vulkan *vulkan, uint32_t *imageIndex, VkSemaphore semaphore);
VkResult avdVulkanSwapchainPresent(AVD_Vulkan *vulkan, uint32_t imageIndex, VkSemaphore waitSemaphore);

bool avdVulkanRendererCreate(AVD_Vulkan *vulkan);
void avdVulkanRendererDestroy(AVD_Vulkan *vulkan);
void avdVulkanRendererRender(AVD_GameState *gameState);

bool avdVulkanPresentationInit(AVD_Vulkan *gameState);
void avdVulkanPresentationDestroy(AVD_Vulkan *gameState); 
bool avdVulkanPresentationRender(AVD_Vulkan *gameState, uint32_t imageIndex);

bool avdVulkanSceneInit(AVD_GameState *gameState);
void avdVulkanSceneDestroy(AVD_GameState *gameState); 
bool avdVulkanSceneRender(AVD_GameState *gameState, uint32_t imageIndex);

bool avdVulkanFormatIsDepth(VkFormat format);
bool avdVulkanFormatIsStencil(VkFormat format);
bool avdVulkanFormatIsDepthStencil(VkFormat format);
bool avdVulkanFramebufferCreate(AVD_Vulkan *vulkan, AVD_VulkanFramebuffer *framebuffer, int32_t width, int32_t height, bool hasDepthStencil, VkFormat colorFormat, VkFormat depthStencilFormat);
void avdVulkanFramebufferDestroy(AVD_Vulkan *vulkan, AVD_VulkanFramebuffer *framebuffer);

bool avdVulkanBufferCreate(AVD_Vulkan* vulkan, AVD_VulkanBuffer *buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
void avdVulkanBufferDestroy(AVD_Vulkan* vulkan, AVD_VulkanBuffer *buffer);

bool avdVulkanImageCreate(AVD_Vulkan* vulkan, AVD_VulkanImage *image, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height);
bool avdVulkanImageTransitionLayout(AVD_VulkanImage *image, VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
bool avdVulkanImageTransitionLayoutWithoutCommandBuffer(AVD_Vulkan* vulkan, AVD_VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
void avdVulkanImageDestroy(AVD_Vulkan* vulkan, AVD_VulkanImage *image);
bool avdVulkanBufferMap(AVD_Vulkan* vulkan, AVD_VulkanBuffer *buffer, void **data);
void avdVulkanBufferUnmap(AVD_Vulkan* vulkan, AVD_VulkanBuffer *buffer);
bool avdVulkanBufferUpload(AVD_Vulkan* vulkan, AVD_VulkanBuffer *buffer, const void *srcData, VkDeviceSize size);
bool avdVulkanImageUploadSimple(AVD_Vulkan* vulkan, AVD_VulkanImage *image, const void *srcData);
bool avdVulkanImageLoadFromFile(AVD_Vulkan* vulkan, const char *filename, AVD_VulkanImage *image);
bool avdVulkanImageLoadFromMemory(AVD_Vulkan* vulkan, const void *data, size_t dataSize, AVD_VulkanImage *image);
bool avdVulkanImageLoadFromAsset(AVD_Vulkan* vulkan, const char *asset, AVD_VulkanImage *image);

#endif // AVD_VULKAN_H