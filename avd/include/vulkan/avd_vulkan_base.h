#ifndef AVD_VULKAN_CORE_H
#define AVD_VULKAN_CORE_H

#define API_VERSION VK_API_VERSION_1_4
#include "volk.h"
#include "vulkan/vk_enum_string_helper.h"

#include "core/avd_core.h"

// third party includes
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#ifndef AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE
#define AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE 1024
#endif

#ifdef AVD_DEBUG

#ifndef AVD_VULKAN_CMD_LABEL_DEFAULT_COLOR
#define AVD_VULKAN_CMD_LABEL_DEFAULT_COLOR \
    (float[]){0.0f, 1.0f, 0.0f, 1.0f}
#endif

#ifndef AVD_VULKAN_QUEUE_LABEL_DEFAULT_COLOR
#define AVD_VULKAN_QUEUE_LABEL_DEFAULT_COLOR \
    (float[]){0.0f, 0.0f, 1.0f, 1.0f}
#endif

#endif

typedef enum {
    AVD_VULKAN_DESCRIPTOR_TYPE_SAMPLER = 0,
    AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    AVD_VULKAN_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    AVD_VULKAN_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    AVD_VULKAN_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    AVD_VULKAN_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    AVD_VULKAN_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    AVD_VULKAN_DESCRIPTOR_TYPE_COUNT
} AVD_VulkanDescriptorType;

typedef struct AVD_VulkanFeatures {
    bool rayTracing;
    bool videoCore;
    bool videoDecode;
    bool videoEncode;
    bool ycbcrConversion;

    VkVideoCapabilitiesKHR videoCapabilitiesDecode;
    VkVideoDecodeCapabilitiesKHR videoDecodeCapabilities;
    VkVideoDecodeH264CapabilitiesKHR videoDecodeH264Capabilities;

    VkVideoCapabilitiesKHR videoCapabilitiesEncode;
    VkVideoEncodeCapabilitiesKHR videoEncodeCapabilities;
    VkVideoEncodeH264CapabilitiesKHR videoEncodeH264Capabilities;
} AVD_VulkanFeatures;

typedef struct {
    bool layersEnabled;
    VkDebugUtilsMessengerEXT debugMessenger;
} AVD_VulkanDebugger;

typedef struct AVD_Vulkan {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue videoDecodeQueue;
    VkQueue videoEncodeQueue;

    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;
    VkCommandPool videoDecodeCommandPool;
    VkCommandPool videoEncodeCommandPool;

    VkDescriptorPool descriptorPool;
    VkDescriptorPool bindlessDescriptorPool;
    VkDescriptorSet bindlessDescriptorSet;
    VkDescriptorSetLayout bindlessDescriptorSetLayout;

    int32_t graphicsQueueFamilyIndex;
    int32_t computeQueueFamilyIndex;
    int32_t videoDecodeQueueFamilyIndex;
    int32_t videoEncodeQueueFamilyIndex;

    AVD_VulkanFeatures supportedFeatures;

#ifdef AVD_DEBUG
    AVD_VulkanDebugger debugger;
#endif
} AVD_Vulkan;

AVD_Vulkan *avdVulkanGetGlobalInstance();

bool avdVulkanInit(AVD_Vulkan *vulkan, AVD_Window *window, VkSurfaceKHR *surface);
void avdVulkanShutdown(AVD_Vulkan *vulkan);
void avdVulkanWaitIdle(AVD_Vulkan *vulkan);
void avdVulkanDestroySurface(AVD_Vulkan *vulkan, VkSurfaceKHR surface);

bool avdVulkanInstanceLayersSupported(const char **layers, uint32_t layerCount);
uint32_t avdVulkanFindMemoryType(AVD_Vulkan *vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkDescriptorType avdVulkanToVkDescriptorType(AVD_VulkanDescriptorType type);

#ifdef AVD_DEBUG
bool avdVulkanAddDebugUtilsExtensions(uint32_t *extensionCount, const char **extensions);
bool avdVulkanAddDebugLayers(uint32_t *layerCount, const char **layers, bool *debugLayersEnabled);

bool avdVulkanDebuggerCreate(AVD_Vulkan *vulkan);
void avdVulkanDebuggerDestroy(AVD_Vulkan *vulkan);

bool avdVulkanDebuggerCmdBeginLabel(AVD_Vulkan *vulkan, VkCommandBuffer commandBuffer, float *color, const char *labelName, ...);
bool avdVulkanDebuggerCmdInsertLabel(AVD_Vulkan *vulkan, VkCommandBuffer commandBuffer, float *color, const char *labelName, ...);
bool avdVulkanDebuggerCmdEndLabel(AVD_Vulkan *vulkan, VkCommandBuffer commandBuffer);

bool avdVulkanDebuggerQueueBeginLabel(AVD_Vulkan *vulkan, VkQueue queue, float *color, const char *labelName, ...);
bool avdVulkanDebuggerQueueInsertLabel(AVD_Vulkan *vulkan, VkQueue queue, float *color, const char *labelName, ...);
bool avdVulkanDebuggerQueueEndLabel(AVD_Vulkan *vulkan, VkQueue queue);

bool avdVulkanDebuggerSetObjectName(AVD_Vulkan *vulkan, VkObjectType objectType, uint64_t objectHandle, const char *name, ...);
#endif

#define AVD_DEBUG_VK_CMD_BEGIN_LABEL(cmdBuffer, color, labelName, ...) \
    AVD_DEBUG_ONLY(avdVulkanDebuggerCmdBeginLabel(avdVulkanGetGlobalInstance(), cmdBuffer, color, labelName, ##__VA_ARGS__))
#define AVD_DEBUG_VK_CMD_INSERT_LABEL(cmdBuffer, color, labelName, ...) \
    AVD_DEBUG_ONLY(avdVulkanDebuggerCmdInsertLabel(avdVulkanGetGlobalInstance(), cmdBuffer, color, labelName, ##__VA_ARGS__))
#define AVD_DEBUG_VK_CMD_END_LABEL(cmdBuffer) \
    AVD_DEBUG_ONLY(avdVulkanDebuggerCmdEndLabel(avdVulkanGetGlobalInstance(), cmdBuffer))
#define AVD_DEBUG_VK_QUEUE_BEGIN_LABEL(queue, color, labelName, ...) \
    AVD_DEBUG_ONLY(avdVulkanDebuggerQueueBeginLabel(avdVulkanGetGlobalInstance(), queue, color, labelName, ##__VA_ARGS__))
#define AVD_DEBUG_VK_QUEUE_INSERT_LABEL(queue, color, labelName, ...) \
    AVD_DEBUG_ONLY(avdVulkanDebuggerQueueInsertLabel(avdVulkanGetGlobalInstance(), queue, color, labelName, ##__VA_ARGS__))
#define AVD_DEBUG_VK_QUEUE_END_LABEL(queue) \
    AVD_DEBUG_ONLY(avdVulkanDebuggerQueueEndLabel(avdVulkanGetGlobalInstance(), queue))

#define AVD_DEBUG_VK_SET_OBJECT_NAME(objectType, objectHandle, name, ...) \
    AVD_DEBUG_ONLY(avdVulkanDebuggerSetObjectName(avdVulkanGetGlobalInstance(), objectType, (uint64_t)objectHandle, name, ##__VA_ARGS__))

#endif // AVD_VULKAN_CORE_H
