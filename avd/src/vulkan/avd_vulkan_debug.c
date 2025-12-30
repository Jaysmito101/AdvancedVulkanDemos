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

}

void avdVulkanDebuggerDestroy(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    if (vulkan->debugger.layersEnabled) {
        vkDestroyDebugUtilsMessengerEXT(vulkan->instance, vulkan->debugger.debugMessenger, NULL);
    }
}

#endif