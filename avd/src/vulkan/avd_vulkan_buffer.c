#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/video/avd_vulkan_video_core.h"

bool avdVulkanBufferCreate(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const char *label)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(buffer != NULL);
    AVD_ASSERT(size > 0);

    snprintf(buffer->label, sizeof(buffer->label), "Buffer/%s/%zu", label ? label : "Unnamed", size);

    VkBufferCreateInfo bufferInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // Assuming exclusive for simplicity
    };

    VkVideoProfileListInfoKHR videoProfileListInfo = {0};
    videoProfileListInfo.sType                     = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR;
    bool isVideoDecodeBuffer                       = usage & VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR || usage & VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR;
    bool isVideoEncodeBuffer                       = usage & VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR || usage & VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR;
    if (isVideoDecodeBuffer || isVideoEncodeBuffer) {
        videoProfileListInfo.profileCount = 1;
        if (isVideoEncodeBuffer) {
            videoProfileListInfo.pProfiles = avdVulkanVideoGetH264EncodeProfileInfo(NULL);
        } else {
            videoProfileListInfo.pProfiles = avdVulkanVideoGetH264DecodeProfileInfo(NULL);
        }
        videoProfileListInfo.pNext = NULL;
        bufferInfo.pNext           = &videoProfileListInfo;

        if (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            AVD_LOG_WARN("Forcing HOST_VISIBLE memory property for video decode/encode buffers");
        }
        // This is a workaround for nvidia video bitstream buffer which, when used for video decode and not mapped,
        // gives incorrect decoding results.
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    } else {
        bufferInfo.pNext = NULL;
    }

    VkResult result = vkCreateBuffer(vulkan->device, &bufferInfo, NULL, &buffer->buffer);
    AVD_CHECK_VK_RESULT(result, "Failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkan->device, buffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = avdVulkanFindMemoryType(vulkan, memRequirements.memoryTypeBits, properties),
    };

    AVD_CHECK_MSG(allocInfo.memoryTypeIndex != UINT32_MAX, "Failed to find suitable memory type for buffer!");

    result = vkAllocateMemory(vulkan->device, &allocInfo, NULL, &buffer->memory);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate buffer memory!");

    result = vkBindBufferMemory(vulkan->device, buffer->buffer, buffer->memory, 0);
    AVD_CHECK_VK_RESULT(result, "Failed to bind buffer memory!");

    buffer->descriptorBufferInfo.buffer = buffer->buffer;
    buffer->descriptorBufferInfo.offset = 0;
    buffer->descriptorBufferInfo.range  = size;
    buffer->usage                       = usage;
    buffer->size                        = size;
    buffer->hostVisible                 = (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    buffer->hostCoherent                = (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;

    return true;
}

void avdVulkanBufferDestroy(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(buffer != NULL);

    vkDestroyBuffer(vulkan->device, buffer->buffer, NULL);
    vkFreeMemory(vulkan->device, buffer->memory, NULL);

    memset(buffer, 0, sizeof(AVD_VulkanBuffer));
}

bool avdVulkanBufferMap(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, void **data)
{
    VkResult result = vkMapMemory(vulkan->device, buffer->memory, 0, VK_WHOLE_SIZE, 0, data);
    AVD_CHECK_VK_RESULT(result, "Failed to map buffer memory!");
    return true;
}

void avdVulkanBufferUnmap(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer)
{
    // auto flush if not host coherent
    if (!buffer->hostCoherent) {
        avdVulkanBufferFlush(vulkan, buffer, buffer->size, 0);
    }

    vkUnmapMemory(vulkan->device, buffer->memory);
}

bool avdVulkanBufferUpload(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, const void *srcData, VkDeviceSize size)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(buffer != NULL);
    AVD_ASSERT(srcData != NULL);

    if (buffer->hostVisible) {
        void *mapped = NULL;
        if (!avdVulkanBufferMap(vulkan, buffer, &mapped)) {
            return false;
        }
        memcpy(mapped, srcData, size);
        avdVulkanBufferUnmap(vulkan, buffer);
        return true;
    }

    AVD_VulkanBuffer staging = {0};
    AVD_CHECK(avdVulkanBufferCreate(vulkan, &staging, size,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    "Core/Buffer/Upload/Staging"));

    void *mapped = NULL;
    if (!avdVulkanBufferMap(vulkan, &staging, &mapped)) {
        avdVulkanBufferDestroy(vulkan, &staging);
        return false;
    }
    memcpy(mapped, srcData, size);
    avdVulkanBufferUnmap(vulkan, &staging);

    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = vulkan->graphicsCommandPool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vulkan->device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &beginInfo);
    AVD_DEBUG_VK_CMD_BEGIN_LABEL(cmd, NULL, "Core/Buffer/Upload");

    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size      = size,
    };
    vkCmdCopyBuffer(cmd, staging.buffer, buffer->buffer, 1, &copyRegion);

    AVD_DEBUG_VK_CMD_END_LABEL(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
    };
    AVD_DEBUG_VK_QUEUE_BEGIN_LABEL(vulkan->graphicsQueue, NULL, "Core/Queue/BufferUpload");
    vkQueueSubmit(vulkan->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    AVD_DEBUG_VK_QUEUE_END_LABEL(vulkan->graphicsQueue);
    vkQueueWaitIdle(vulkan->graphicsQueue);

    vkFreeCommandBuffers(vulkan->device, vulkan->graphicsCommandPool, 1, &cmd);
    avdVulkanBufferDestroy(vulkan, &staging);
    return true;
}

bool avdVulkanBufferFlush(AVD_Vulkan *vulkan, AVD_VulkanBuffer *buffer, VkDeviceSize size, VkDeviceSize offset)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(buffer != NULL);

    if (!buffer->hostVisible) {
        AVD_LOG_ERROR("Cannot flush a non-host-visible buffer");
        return false;
    }

    if (buffer->hostCoherent) {
        return true;
    }

    VkMappedMemoryRange mappedRange = {
        .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = buffer->memory,
        .offset = offset,
        .size   = size,
    };

    VkResult result = vkFlushMappedMemoryRanges(vulkan->device, 1, &mappedRange);
    AVD_CHECK_VK_RESULT(result, "Failed to flush mapped memory ranges!");

    return true;
}