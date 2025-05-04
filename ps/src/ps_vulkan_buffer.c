#include "ps_vulkan.h"
#include "ps_game_state.h"

bool psVulkanBufferCreate(PS_GameState *gameState, PS_VulkanBuffer *buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Assuming exclusive for simplicity

    if (vkCreateBuffer(gameState->vulkan.device, &bufferInfo, NULL, &buffer->buffer) != VK_SUCCESS)
    {
        PS_LOG("Failed to create buffer!\n");
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(gameState->vulkan.device, buffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = psVulkanFindMemoryType(gameState, memRequirements.memoryTypeBits, properties);

    if (allocInfo.memoryTypeIndex == UINT32_MAX)
    {
        PS_LOG("Failed to find suitable memory type for buffer!\n");
        vkDestroyBuffer(gameState->vulkan.device, buffer->buffer, NULL);
        buffer->buffer = VK_NULL_HANDLE;
        return false;
    }

    if (vkAllocateMemory(gameState->vulkan.device, &allocInfo, NULL, &buffer->memory) != VK_SUCCESS)
    {
        PS_LOG("Failed to allocate buffer memory!\n");
        vkDestroyBuffer(gameState->vulkan.device, buffer->buffer, NULL);
        buffer->buffer = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(gameState->vulkan.device, buffer->buffer, buffer->memory, 0);

    buffer->descriptorBufferInfo.buffer = buffer->buffer;
    buffer->descriptorBufferInfo.offset = 0;
    buffer->descriptorBufferInfo.range = size;

    return true;
}

void psVulkanBufferDestroy(PS_GameState *gameState, PS_VulkanBuffer *buffer)
{
    if (buffer->buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(gameState->vulkan.device, buffer->buffer, NULL);
        buffer->buffer = VK_NULL_HANDLE;
    }
    if (buffer->memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(gameState->vulkan.device, buffer->memory, NULL);
        buffer->memory = VK_NULL_HANDLE;
    }
}

bool psVulkanBufferMap(PS_GameState *gameState, PS_VulkanBuffer *buffer, void **data) {
    if (vkMapMemory(gameState->vulkan.device, buffer->memory, 0, VK_WHOLE_SIZE, 0, data) != VK_SUCCESS) {
        PS_LOG("Failed to map buffer memory!\n");
        return false;
    }
    return true;
}

void psVulkanBufferUnmap(PS_GameState *gameState, PS_VulkanBuffer *buffer) {
    vkUnmapMemory(gameState->vulkan.device, buffer->memory);
}

bool psVulkanBufferUpload(PS_GameState *gameState, PS_VulkanBuffer *buffer, const void *srcData, VkDeviceSize size) {
    PS_VulkanBuffer staging = {0};
    if (!psVulkanBufferCreate(gameState, &staging, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return false;
    }

    void *mapped = NULL;
    if (!psVulkanBufferMap(gameState, &staging, &mapped)) {
        psVulkanBufferDestroy(gameState, &staging);
        return false;
    }
    memcpy(mapped, srcData, size);
    psVulkanBufferUnmap(gameState, &staging);

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = gameState->vulkan.graphicsCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(gameState->vulkan.device, &allocInfo, &cmd);

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
    vkQueueSubmit(gameState->vulkan.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(gameState->vulkan.graphicsQueue);

    vkFreeCommandBuffers(gameState->vulkan.device, gameState->vulkan.graphicsCommandPool, 1, &cmd);
    psVulkanBufferDestroy(gameState, &staging);
    return true;
}