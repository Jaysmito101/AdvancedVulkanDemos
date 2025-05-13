#include "ps_vulkan.h"
#include "ps_types.h"

bool psVulkanBufferCreate(PS_Vulkan *vulkan, PS_VulkanBuffer *buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Assuming exclusive for simplicity

    VkResult result = vkCreateBuffer(vulkan->device, &bufferInfo, NULL, &buffer->buffer);
    PS_CHECK_VK_RESULT(result, "Failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkan->device, buffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = psVulkanFindMemoryType(vulkan, memRequirements.memoryTypeBits, properties);

    PS_CHECK_MSG(allocInfo.memoryTypeIndex != UINT32_MAX, "Failed to find suitable memory type for buffer!");

    result = vkAllocateMemory(vulkan->device, &allocInfo, NULL, &buffer->memory);
    PS_CHECK_VK_RESULT(result, "Failed to allocate buffer memory!");

    result = vkBindBufferMemory(vulkan->device, buffer->buffer, buffer->memory, 0);
    PS_CHECK_VK_RESULT(result, "Failed to bind buffer memory!");

    buffer->descriptorBufferInfo.buffer = buffer->buffer;
    buffer->descriptorBufferInfo.offset = 0;
    buffer->descriptorBufferInfo.range = size;

    return true;
}

void psVulkanBufferDestroy(PS_Vulkan *vulkan, PS_VulkanBuffer *buffer)
{
    vkDestroyBuffer(vulkan->device, buffer->buffer, NULL);
    vkFreeMemory(vulkan->device, buffer->memory, NULL);
}

bool psVulkanBufferMap(PS_Vulkan *vulkan, PS_VulkanBuffer *buffer, void **data)
{
    VkResult result = vkMapMemory(vulkan->device, buffer->memory, 0, VK_WHOLE_SIZE, 0, data);
    PS_CHECK_VK_RESULT(result, "Failed to map buffer memory!");
    return true;
}

void psVulkanBufferUnmap(PS_Vulkan *vulkan, PS_VulkanBuffer *buffer)
{
    vkUnmapMemory(vulkan->device, buffer->memory);
}

bool psVulkanBufferUpload(PS_Vulkan *vulkan, PS_VulkanBuffer *buffer, const void *srcData, VkDeviceSize size)
{
    PS_VulkanBuffer staging = {0};
    PS_CHECK(psVulkanBufferCreate(vulkan, &staging, size,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    void *mapped = NULL;
    if (!psVulkanBufferMap(vulkan, &staging, &mapped))
    {
        psVulkanBufferDestroy(vulkan, &staging);
        return false;
    }
    memcpy(mapped, srcData, size);
    psVulkanBufferUnmap(vulkan, &staging);

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
    psVulkanBufferDestroy(vulkan, &staging);
    return true;
}