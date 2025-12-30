#ifndef AVD_VULKAN_BUFFER_H
#define AVD_VULKAN_BUFFER_H

#include "vulkan/avd_vulkan_base.h"

typedef struct AVD_VulkanBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDescriptorBufferInfo descriptorBufferInfo;
    VkBufferUsageFlags usage;
    VkDeviceSize size;
    char label[64];
} AVD_VulkanBuffer;

bool avdVulkanBufferCreate(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const char *label);
void avdVulkanBufferDestroy(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer);
bool avdVulkanBufferMap(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, void **data);
void avdVulkanBufferUnmap(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer);
bool avdVulkanBufferUpload(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, const void *srcData, VkDeviceSize size);

#endif // AVD_VULKAN_BUFFER_H