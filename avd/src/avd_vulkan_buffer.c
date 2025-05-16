#include "avd_vulkan.h"
#include "avd_types.h"

bool avdVulkanBufferCreate(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Assuming exclusive for simplicity

    VkResult result = vkCreateBuffer(vulkan->device, &bufferInfo, NULL, &buffer->buffer);
    AVD_CHECK_VK_RESULT(result, "Failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkan->device, buffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = avdVulkanFindMemoryType(vulkan, memRequirements.memoryTypeBits, properties);

    AVD_CHECK_MSG(allocInfo.memoryTypeIndex != UINT32_MAX, "Failed to find suitable memory type for buffer!");

    result = vkAllocateMemory(vulkan->device, &allocInfo, NULL, &buffer->memory);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate buffer memory!");

    result = vkBindBufferMemory(vulkan->device, buffer->buffer, buffer->memory, 0);
    AVD_CHECK_VK_RESULT(result, "Failed to bind buffer memory!");

    buffer->descriptorBufferInfo.buffer = buffer->buffer;
    buffer->descriptorBufferInfo.offset = 0;
    buffer->descriptorBufferInfo.range = size;

    return true;
}

void avdVulkanBufferDestroy(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer)
{
    vkDestroyBuffer(vulkan->device, buffer->buffer, NULL);
    vkFreeMemory(vulkan->device, buffer->memory, NULL);
}

bool avdVulkanBufferMap(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, void **data)
{
    VkResult result = vkMapMemory(vulkan->device, buffer->memory, 0, VK_WHOLE_SIZE, 0, data);
    AVD_CHECK_VK_RESULT(result, "Failed to map buffer memory!");
    return true;
}

void avdVulkanBufferUnmap(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer)
{
    vkUnmapMemory(vulkan->device, buffer->memory);
}

bool avdVulkanBufferUpload(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, const void *srcData, VkDeviceSize size)
{
    AVD_VulkanBuffer staging = {0};
    AVD_CHECK(avdVulkanBufferCreate(vulkan, &staging, size,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    void *mapped = NULL;
    if (!avdVulkanBufferMap(vulkan, &staging, &mapped))
    {
        avdVulkanBufferDestroy(vulkan, &staging);
        return false;
    }
    memcpy(mapped, srcData, size);
    avdVulkanBufferUnmap(vulkan, &staging);

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vulkan->graphicsCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vulkan->device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegion = {0};
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, staging.buffer, buffer->buffer, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(vulkan->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkan->graphicsQueue);

    vkFreeCommandBuffers(vulkan->device, vulkan->graphicsCommandPool, 1, &cmd);
    avdVulkanBufferDestroy(vulkan, &staging);
    return true;
}