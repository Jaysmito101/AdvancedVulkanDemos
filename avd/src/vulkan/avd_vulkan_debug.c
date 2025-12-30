#include "core/avd_base.h"
#include "vulkan/avd_vulkan_base.h"

#ifdef AVD_DEBUG

static VkBool32 __avdDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                 const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                 void *pUserData)
{
    const char *severity = NULL;
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severity = "VERBOSE";
            AVD_LOG_DEBUG("%s: %s", severity, pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severity = "INFO";
            AVD_LOG_INFO("%s: %s", severity, pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severity = "WARNING";
            AVD_LOG_WARN("%s: %s", severity, pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severity = "ERROR";
            AVD_LOG_ERROR("%s: %s", severity, pCallbackData->pMessage);
            break;
        default:
            severity = "UNKNOWN";
            AVD_LOG_VERBOSE("%s: %s", severity, pCallbackData->pMessage);
            break;
    }
    return VK_FALSE; // Don't abort on debug messages
}

static bool __avdCreateDebugUtilsMessenger(AVD_Vulkan *vulkan, AVD_VulkanDebugger *debugger)
{
    AVD_ASSERT(vulkan != NULL);

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = __avdDebugUtilsMessengerCallback,
        .pUserData       = vulkan,
    };

    AVD_CHECK_VK_RESULT(
        vkCreateDebugUtilsMessengerEXT(
            vulkan->instance,
            &createInfo,
            NULL,
            &debugger->debugMessenger),
        "Failed to create debug utils messenger\n");
    return true;
}

bool avdVulkanAddDebugUtilsExtensions(uint32_t *extensionCount, const char **extensions)
{
    extensions[*extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    (*extensionCount)++;
    return true;
}

bool avdVulkanAddDebugLayers(uint32_t *layerCount, const char **layers, bool *debugLayersEnabled)
{
    static const char *debugLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    if (avdVulkanInstanceLayersSupported(debugLayers, AVD_ARRAY_COUNT(debugLayers))) {
        for (uint32_t i = 0; i < AVD_ARRAY_COUNT(debugLayers); ++i) {
            layers[i + *layerCount] = debugLayers[i];
        }
        *layerCount += AVD_ARRAY_COUNT(debugLayers);
        *debugLayersEnabled = true;
    } else {
        AVD_LOG_WARN("Debug layers not supported");
    }
    return true;
}

bool avdVulkanDebuggerCreate(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    if (vulkan->debugger.layersEnabled) {
        AVD_CHECK(__avdCreateDebugUtilsMessenger(vulkan, &vulkan->debugger));
    }
    return true;
}

void avdVulkanDebuggerDestroy(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    if (vulkan->debugger.layersEnabled) {
        vkDestroyDebugUtilsMessengerEXT(vulkan->instance, vulkan->debugger.debugMessenger, NULL);
    }
}

bool avdVulkanDebuggerCmdBeginLabel(AVD_Vulkan *vulkan, VkCommandBuffer commandBuffer, const char *labelName, float *colorIn)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(labelName != NULL);
    AVD_ASSERT(vulkan->debugger.layersEnabled);

    float *color = colorIn != NULL ? colorIn : AVD_VULKAN_CMD_LABEL_DEFAULT_COLOR;

    VkDebugUtilsLabelEXT labelInfo = {
        .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = labelName,
        .color      = {color[0], color[1], color[2], color[3]},
    };

    vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
    return true;
}

bool avdVulkanDebuggerCmdInsertLabel(AVD_Vulkan *vulkan, VkCommandBuffer commandBuffer, const char *labelName, float *colorIn)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(labelName != NULL);
    AVD_ASSERT(vulkan->debugger.layersEnabled);

    float *color = colorIn != NULL ? colorIn : AVD_VULKAN_CMD_LABEL_DEFAULT_COLOR;

    VkDebugUtilsLabelEXT labelInfo = {
        .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = labelName,
        .color      = {color[0], color[1], color[2], color[3]},
    };

    vkCmdInsertDebugUtilsLabelEXT(commandBuffer, &labelInfo);
    return true;
}

bool avdVulkanDebuggerCmdEndLabel(AVD_Vulkan *vulkan, VkCommandBuffer commandBuffer)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(vulkan->debugger.layersEnabled);

    vkCmdEndDebugUtilsLabelEXT(commandBuffer);
    return true;
}

bool avdVulkanDebuggerQueueBeginLabel(AVD_Vulkan *vulkan, VkQueue queue, const char *labelName, float *colorIn)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(queue != VK_NULL_HANDLE);
    AVD_ASSERT(labelName != NULL);
    AVD_ASSERT(vulkan->debugger.layersEnabled);

    float *color = colorIn != NULL ? colorIn : AVD_VULKAN_CMD_LABEL_DEFAULT_COLOR;

    VkDebugUtilsLabelEXT labelInfo = {
        .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = labelName,
        .color      = {color[0], color[1], color[2], color[3]},
    };

    vkQueueBeginDebugUtilsLabelEXT(queue, &labelInfo);
    return true;
}

bool avdVulkanDebuggerQueueInsertLabel(AVD_Vulkan *vulkan, VkQueue queue, const char *labelName, float *colorIn)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(queue != VK_NULL_HANDLE);
    AVD_ASSERT(labelName != NULL);
    AVD_ASSERT(vulkan->debugger.layersEnabled);

    float *color = colorIn != NULL ? colorIn : AVD_VULKAN_CMD_LABEL_DEFAULT_COLOR;

    VkDebugUtilsLabelEXT labelInfo = {
        .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = labelName,
        .color      = {color[0], color[1], color[2], color[3]},
    };

    vkQueueInsertDebugUtilsLabelEXT(queue, &labelInfo);
    return true;
}

bool avdVulkanDebuggerQueueEndLabel(AVD_Vulkan *vulkan, VkQueue queue)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(queue != VK_NULL_HANDLE);
    AVD_ASSERT(vulkan->debugger.layersEnabled);

    vkQueueEndDebugUtilsLabelEXT(queue);
    return true;
}

bool avdVulkanDebuggerSetObjectName(AVD_Vulkan *vulkan, VkObjectType objectType, uint64_t objectHandle, const char *name)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(name != NULL);
    AVD_ASSERT(vulkan->debugger.layersEnabled);

    VkDebugUtilsObjectNameInfoEXT nameInfo = {
        .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType   = objectType,
        .objectHandle = objectHandle,
        .pObjectName  = name,
    };

    vkSetDebugUtilsObjectNameEXT(vulkan->device, &nameInfo);
    return true;
}

#endif