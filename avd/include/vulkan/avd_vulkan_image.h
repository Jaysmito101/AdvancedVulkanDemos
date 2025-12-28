#ifndef AVD_VULKAN_IMAGE_H
#define AVD_VULKAN_IMAGE_H

#include "vulkan/avd_vulkan_base.h"

typedef struct {
    VkFormat format;
    VkImageUsageFlags usage;

    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t arrayLayers;
    uint32_t mipLevels;
} AVD_VulkanImageCreateInfo;

typedef struct {
    VkImageView imageView;
    VkImageSubresourceRange subresourceRange;
    VkDescriptorImageInfo descriptorImageInfo;
} AVD_VulkanImageSubresource;

typedef struct {
    VkImage image;
    VkSampler sampler;
    VkDeviceMemory memory;

    AVD_VulkanImageSubresource defaultSubresource;

    AVD_VulkanImageCreateInfo info;
} AVD_VulkanImage;

AVD_VulkanImageCreateInfo avdVulkanImageGetDefaultCreateInfo(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
bool avdVulkanImageCreate(AVD_Vulkan *vulkan, AVD_VulkanImage *image, AVD_VulkanImageCreateInfo createInfo);
bool avdVulkanImageTransitionLayout(AVD_VulkanImage *image, VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, AVD_VulkanImageSubresource *subresourceRange);
bool avdVulkanImageTransitionLayoutWithoutCommandBuffer(AVD_Vulkan *vulkan, AVD_VulkanImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, AVD_VulkanImageSubresource *subresourceRange);
void avdVulkanImageDestroy(AVD_Vulkan *vulkan, AVD_VulkanImage *image);
bool avdVulkanImageUploadSimple(AVD_Vulkan *vulkan, AVD_VulkanImage *image, const void *srcData, AVD_VulkanImageSubresource *subresourceRange);
bool avdVulkanImageLoadFromFile(AVD_Vulkan *vulkan, const char *filename, AVD_VulkanImage *image);
bool avdVulkanImageLoadFromMemory(AVD_Vulkan *vulkan, const void *data, size_t dataSize, AVD_VulkanImage *image);
bool avdVulkanImageLoadFromAsset(AVD_Vulkan *vulkan, const char *asset, AVD_VulkanImage *image);
bool avdVulkanImageSubresourceCreate(AVD_Vulkan *vulkan, AVD_VulkanImage *image, VkImageSubresourceRange subresourceRange, AVD_VulkanImageSubresource *outSubresource);
void avdVulkanImageSubresourceDestroy(AVD_Vulkan *vulkan, AVD_VulkanImageSubresource *subresource);

#endif // AVD_VULKAN_IMAGE_H