#ifndef AVD_VULKAN_IMAGE_H
#define AVD_VULKAN_IMAGE_H

#include "vulkan/avd_vulkan_base.h"

typedef struct AVD_VulkanImage {
    VkImage image;
    VkImageView imageView;
    VkSampler sampler;

    VkDeviceMemory memory;

    VkImageSubresourceRange subresourceRange;
    VkDescriptorImageInfo descriptorImageInfo;
    VkFormat format;

    uint32_t width;
    uint32_t height;
} AVD_VulkanImage;

bool avdVulkanImageCreate(AVD_Vulkan *vulkan, AVD_VulkanImage *image, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height);
bool avdVulkanImageTransitionLayout(AVD_VulkanImage *image, VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
bool avdVulkanImageTransitionLayoutWithoutCommandBuffer(AVD_Vulkan *vulkan, AVD_VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
void avdVulkanImageDestroy(AVD_Vulkan *vulkan, AVD_VulkanImage *image);
bool avdVulkanImageUploadSimple(AVD_Vulkan *vulkan, AVD_VulkanImage *image, const void *srcData);
bool avdVulkanImageLoadFromFile(AVD_Vulkan *vulkan, const char *filename, AVD_VulkanImage *image);
bool avdVulkanImageLoadFromMemory(AVD_Vulkan *vulkan, const void *data, size_t dataSize, AVD_VulkanImage *image);
bool avdVulkanImageLoadFromAsset(AVD_Vulkan *vulkan, const char *asset, AVD_VulkanImage *image);

#endif // AVD_VULKAN_IMAGE_H