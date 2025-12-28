#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"
#include <stdint.h>

static const char *__avd_RequiredVulkanExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
    VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,
    // TODO: We can just use VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME and
    // VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME instead but we are using the
    // EXT versions for better compatibility with older drivers. In the future
    // we can switch to the KHR versions when they are more widely supported.
    VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};

static const char *__avd_VulkanRayTraceExtensions[] = {
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_QUERY_EXTENSION_NAME
    // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, // disabling this as this messes renderdoc
};

static const char *__avd_VulkanVideoExtensions[] = {
    VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
    VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME,
    VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    // VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME, // we only deal with h264 for now!
};

static AVD_VulkanFeatures *__avdVulkanFeaturesInit(AVD_VulkanFeatures *features)
{
    AVD_ASSERT(features != NULL);
    features->rayTracing  = false;
    features->videoDecode = false;
    return features;
}

static bool __avdVulkanLayersSupported(const char **layers, uint32_t layerCount)
{
    static VkLayerProperties availableLayers[128] = {0};
    uint32_t availableLayerCount                  = 0;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
    if (availableLayerCount > AVD_ARRAY_COUNT(availableLayers)) {
        AVD_LOG_WARN("Too many available layers");
        return false;
    }
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);
    for (uint32_t i = 0; i < layerCount; ++i) {
        bool found = false;
        for (uint32_t j = 0; j < availableLayerCount; ++j) {
            if (strcmp(layers[i], availableLayers[j].layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            AVD_LOG_WARN("Layer %s not supported", layers[i]);
            return false;
        }
    }
    return true;
}

static bool __avdAddGlfwExtenstions(uint32_t *extensionCount, const char **extensions)
{
    uint32_t count              = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    if (glfwExtensions == NULL) {
        AVD_LOG_ERROR("Failed to get GLFW extensions");
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        extensions[i + *extensionCount] = glfwExtensions[i];
    }
    *extensionCount += count;
    return true;
}

static void __avdVulkanAddDeviceExtensionsToList(const char **extensions, uint32_t *extensionCount, const char **deviceExtensions, uint32_t deviceExtensionCount)
{
    for (uint32_t i = 0; i < deviceExtensionCount; ++i) {
        extensions[*extensionCount] = deviceExtensions[i];
        (*extensionCount)++;
    }
}

static const char **__avdGetVulkanDeviceExtensions(AVD_Vulkan *vulkan, uint32_t *extensionCount)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(extensionCount != NULL);

    static const char *deviceExtensions[128] = {0};

    uint32_t count = 0;
    __avdVulkanAddDeviceExtensionsToList(deviceExtensions, &count, __avd_RequiredVulkanExtensions, AVD_ARRAY_COUNT(__avd_RequiredVulkanExtensions));
    if (vulkan->supportedFeatures.rayTracing) {
        __avdVulkanAddDeviceExtensionsToList(deviceExtensions, &count, __avd_VulkanRayTraceExtensions, AVD_ARRAY_COUNT(__avd_VulkanRayTraceExtensions));
    }
    if (vulkan->supportedFeatures.videoDecode) {
        __avdVulkanAddDeviceExtensionsToList(deviceExtensions, &count, __avd_VulkanVideoExtensions, AVD_ARRAY_COUNT(__avd_VulkanVideoExtensions));
    }

    *extensionCount = count;
    return deviceExtensions;
}

static bool __avdAddSurfaceExtensions(uint32_t *extensionCount, const char **extensions)
{
    extensions[*extensionCount] = VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME;
    (*extensionCount)++;

    extensions[*extensionCount] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
    (*extensionCount)++;

    return true;
}

#ifdef AVD_DEBUG
static bool __avdAddDebugUtilsExtenstions(uint32_t *extensionCount, const char **extensions)
{
    extensions[*extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    (*extensionCount)++;
    return true;
}

static bool __avdAddDebugLayers(uint32_t *layerCount, const char **layers, bool *debugLayersEnabled)
{
    static const char *debugLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    if (__avdVulkanLayersSupported(debugLayers, AVD_ARRAY_COUNT(debugLayers))) {
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

static bool __avdCreateDebugUtilsMessenger(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
    createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = __avdDebugUtilsMessengerCallback;
    createInfo.pUserData       = vulkan;

    VkResult result = vkCreateDebugUtilsMessengerEXT(vulkan->instance, &createInfo, NULL, &vulkan->debugMessenger);
    AVD_CHECK_VK_RESULT(result, "Failed to create debug utils messenger\n");
    return true;
}

#endif

static bool __avdVulkanCreateInstance(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    VkApplicationInfo appInfo  = {0};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "AVD";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "AVD";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_1;

    uint32_t extensionCount           = 0;
    static const char *extensions[64] = {0};

    AVD_CHECK(__avdAddGlfwExtenstions(&extensionCount, extensions));
    AVD_CHECK(__avdAddSurfaceExtensions(&extensionCount, extensions));

#ifdef AVD_DEBUG
    AVD_CHECK(__avdAddDebugUtilsExtenstions(&extensionCount, extensions));
#endif

    uint32_t layerCount           = 0;
    static const char *layers[64] = {0};

#ifdef AVD_DEBUG
    AVD_CHECK(__avdAddDebugLayers(&layerCount, layers, &vulkan->debugLayersEnabled));
#endif

    VkInstanceCreateInfo createInfo    = {0};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledLayerCount       = layerCount;
    createInfo.ppEnabledLayerNames     = layers;
    createInfo.enabledExtensionCount   = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, NULL, &vulkan->instance);
    AVD_CHECK_VK_RESULT(result, "Failed to create Vulkan instance\n");

    volkLoadInstance(vulkan->instance);

#ifdef AVD_DEBUG
    if (vulkan->debugLayersEnabled) {
        AVD_CHECK(__avdCreateDebugUtilsMessenger(vulkan));
    }
#endif

    return true;
}

static bool __avdVulkanCreateSurface(AVD_Vulkan *vulkan, GLFWwindow *window, VkSurfaceKHR *surface)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(window != NULL);
    AVD_ASSERT(surface != NULL);

    VkResult result = glfwCreateWindowSurface(vulkan->instance, window, NULL, surface);
    AVD_CHECK_VK_RESULT(result, "Failed to create window surface\n");
    return true;
}

static bool __avdVulkanPhysicalDeviceCheckExtensionsSet(VkExtensionProperties *extensions, uint32_t extensionsCount, const char **requiredExtensions, uint32_t requiredExtensionCount)
{
    for (uint32_t i = 0; i < requiredExtensionCount; ++i) {
        bool found = false;
        for (uint32_t j = 0; j < extensionsCount; ++j) {
            if (strcmp(requiredExtensions[i], extensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

static bool __avdVulkanPhysicalDeviceCheckExtensions(VkPhysicalDevice device, AVD_VulkanFeatures *outFeatures)
{
    uint32_t extensionCount                      = 0;
    static VkExtensionProperties extensions[256] = {0};
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
    if (extensionCount == 0) {
        AVD_LOG_ERROR("No Vulkan-compatible extensions found");
        return false;
    }
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, extensions);

    if (!__avdVulkanPhysicalDeviceCheckExtensionsSet(extensions, extensionCount, __avd_RequiredVulkanExtensions, AVD_ARRAY_COUNT(__avd_RequiredVulkanExtensions))) {
        return false;
    }

    outFeatures->rayTracing  = __avdVulkanPhysicalDeviceCheckExtensionsSet(extensions, extensionCount, __avd_VulkanRayTraceExtensions, AVD_ARRAY_COUNT(__avd_VulkanRayTraceExtensions));
    outFeatures->videoDecode = __avdVulkanPhysicalDeviceCheckExtensionsSet(extensions, extensionCount, __avd_VulkanVideoExtensions, AVD_ARRAY_COUNT(__avd_VulkanVideoExtensions));

    return true;
}

static bool __avdVulkanPhysicalDeviceCheckFeatures(VkPhysicalDevice device, AVD_VulkanFeatures *outFeatures)
{
    VkPhysicalDeviceFeatures features = {0};
    vkGetPhysicalDeviceFeatures(device, &features);

    if (!features.samplerAnisotropy) {
        AVD_LOG_WARN("Sampler anisotropy not supported");
        return false;
    }

    // TODO: Add all the feature checks here! (NOW ITS INCOMPLETE)

    return true;
}

static bool __avdVulkanPickPhysicalDevice(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkan->instance, &deviceCount, NULL);
    AVD_CHECK_MSG(deviceCount > 0, "No Vulkan-compatible devices found\n");

    deviceCount                  = AVD_MIN(deviceCount, 64);
    VkPhysicalDevice devices[64] = {0};
    vkEnumeratePhysicalDevices(vulkan->instance, &deviceCount, devices);

    AVD_VulkanFeatures supportedFeatures = {0};
    bool foundDiscreteGPU                = false;
    for (uint32_t i = 0; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        if (!__avdVulkanPhysicalDeviceCheckExtensions(devices[i], __avdVulkanFeaturesInit(&supportedFeatures))) {
            AVD_LOG_WARN("Physical device %s does not support required extensions\n", deviceProperties.deviceName);
            continue;
        }

        if (!__avdVulkanPhysicalDeviceCheckFeatures(devices[i], &supportedFeatures)) {
            AVD_LOG_WARN("Physical device %s does not support required features\n", deviceProperties.deviceName);
            continue;
        }

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            vulkan->physicalDevice    = devices[i];
            vulkan->supportedFeatures = supportedFeatures;
            foundDiscreteGPU          = true;
            break;
        }
    }

    AVD_CHECK_MSG(foundDiscreteGPU, "No suitable physical device found\n");

    VkPhysicalDeviceProperties deviceProperties = {0};
    vkGetPhysicalDeviceProperties(vulkan->physicalDevice, &deviceProperties);
    AVD_LOG_INFO("Selected physical device: %s", deviceProperties.deviceName);

    return true;
}

static int32_t __avdVulkanFindQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags flags, VkSurfaceKHR surface, int32_t excludeIndex)
{
    uint32_t queueFamilyCount                         = 0;
    static VkQueueFamilyProperties queueFamilies[128] = {0};

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    if (queueFamilyCount == 0) {
        AVD_LOG_ERROR("No Vulkan-compatible queue families found");
        return -1;
    }

    if (queueFamilyCount > AVD_ARRAY_COUNT(queueFamilies)) {
        AVD_LOG_WARN("Too many Vulkan-compatible queue families found");
        return -1;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (i == excludeIndex) {
            continue;
        }
        if ((queueFamilies[i].queueFlags & flags) == flags) {
            if (surface != VK_NULL_HANDLE) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (!presentSupport) {
                    continue;
                }
            }
            return i;
        }
    }
    return -1;
}

static bool __avdVulkanCreateDevice(AVD_Vulkan *vulkan, VkSurfaceKHR *surface)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(surface != NULL);

    int32_t graphicsQueueFamilyIndex = __avdVulkanFindQueueFamilyIndex(vulkan->physicalDevice, VK_QUEUE_GRAPHICS_BIT, *surface, -1);
    AVD_CHECK_MSG(graphicsQueueFamilyIndex >= 0, "Failed to find graphics queue family index\n");

    int32_t computeQueueFamilyIndex = __avdVulkanFindQueueFamilyIndex(vulkan->physicalDevice, VK_QUEUE_COMPUTE_BIT, VK_NULL_HANDLE, graphicsQueueFamilyIndex);
    AVD_CHECK_MSG(computeQueueFamilyIndex >= 0, "Failed to find compute queue family index\n");

    int32_t videoDecodeQueueFamilyIndex = __avdVulkanFindQueueFamilyIndex(vulkan->physicalDevice, VK_QUEUE_VIDEO_DECODE_BIT_KHR | VK_QUEUE_TRANSFER_BIT, VK_NULL_HANDLE, -1);
    if (vulkan->supportedFeatures.videoDecode) {
        AVD_CHECK_MSG(videoDecodeQueueFamilyIndex >= 0, "Failed to find video decode queue family index\n");
    }

    // We only need one graphics queue and one compute queue for now
    VkDeviceQueueCreateInfo queueCreateInfos[3] = {0};
    AVD_Size queueCreateInfoCount               = 0;
    float queuePriority                         = 1.0f;

    //  first the graphics queue
    queueCreateInfos[queueCreateInfoCount].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
    queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
    queueCreateInfoCount++;

    // then the compute queue
    queueCreateInfos[queueCreateInfoCount].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = computeQueueFamilyIndex;
    queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
    queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
    queueCreateInfoCount++;

    if (vulkan->supportedFeatures.videoDecode) {
        // finally the video decode queue
        queueCreateInfos[queueCreateInfoCount].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = videoDecodeQueueFamilyIndex;
        queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
        queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
        queueCreateInfoCount++;
    }

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {0};
    rayTracingPipelineFeatures.sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    // rayTracingPipelineFeatures.rayTracingPipeline                            = VK_TRUE;
    rayTracingPipelineFeatures.pNext = NULL;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {0};
    rayQueryFeatures.sType                               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery                            = VK_TRUE;
    rayQueryFeatures.pNext                               = &rayTracingPipelineFeatures;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures      = {0};
    accelerationStructureFeatures.sType                                                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.accelerationStructure                                 = VK_TRUE;
    accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
    accelerationStructureFeatures.pNext                                                 = &rayQueryFeatures;

    VkPhysicalDeviceVulkan12Features deviceVulkan12Features              = {0};
    deviceVulkan12Features.sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    deviceVulkan12Features.descriptorIndexing                            = VK_TRUE;
    deviceVulkan12Features.bufferDeviceAddress                           = VK_TRUE;
    deviceVulkan12Features.shaderFloat16                                 = VK_TRUE;
    deviceVulkan12Features.shaderInt8                                    = VK_TRUE;
    deviceVulkan12Features.uniformAndStorageBuffer8BitAccess             = VK_TRUE;
    deviceVulkan12Features.storageBuffer8BitAccess                       = VK_TRUE;
    deviceVulkan12Features.drawIndirectCount                             = VK_TRUE;
    deviceVulkan12Features.imagelessFramebuffer                          = VK_TRUE;
    deviceVulkan12Features.runtimeDescriptorArray                        = VK_TRUE;
    deviceVulkan12Features.descriptorBindingPartiallyBound               = VK_TRUE;
    deviceVulkan12Features.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
    deviceVulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    deviceVulkan12Features.descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE;
    deviceVulkan12Features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    deviceVulkan12Features.descriptorBindingUpdateUnusedWhilePending     = VK_TRUE;
    deviceVulkan12Features.descriptorBindingVariableDescriptorCount      = VK_TRUE;
    deviceVulkan12Features.pNext                                         = &accelerationStructureFeatures;

    VkPhysicalDeviceVulkan11Features deviceVulkan11Features   = {0};
    deviceVulkan11Features.sType                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    deviceVulkan11Features.uniformAndStorageBuffer16BitAccess = VK_TRUE;
    deviceVulkan11Features.storageBuffer16BitAccess           = VK_TRUE;
    deviceVulkan11Features.pNext                              = &deviceVulkan12Features;

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1Features = {0};
    swapchainMaintenance1Features.sType                                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
    swapchainMaintenance1Features.swapchainMaintenance1                            = VK_TRUE;
    swapchainMaintenance1Features.pNext                                            = &deviceVulkan11Features;

    VkPhysicalDeviceFeatures2 deviceFeatures2        = {0};
    deviceFeatures2.sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features.multiDrawIndirect       = VK_TRUE;
    deviceFeatures2.features.pipelineStatisticsQuery = VK_TRUE;
    deviceFeatures2.features.samplerAnisotropy       = VK_TRUE;
    deviceFeatures2.features.shaderInt64             = VK_TRUE;
    deviceFeatures2.features.shaderInt16             = VK_TRUE;
    deviceFeatures2.pNext                            = &swapchainMaintenance1Features;

    VkDeviceCreateInfo createInfo      = {0};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = (AVD_UInt32)queueCreateInfoCount;
    createInfo.pQueueCreateInfos       = queueCreateInfos;
    createInfo.ppEnabledExtensionNames = __avdGetVulkanDeviceExtensions(vulkan, &createInfo.enabledExtensionCount);
    createInfo.enabledLayerCount       = 0;
    createInfo.pNext                   = &deviceFeatures2;

#ifdef AVD_DEBUG
    if (vulkan->debugLayersEnabled) {
        static const char *debugLayers[] = {
            "VK_LAYER_KHRONOS_validation",
        };
        uint32_t debugLayerCount       = AVD_ARRAY_COUNT(debugLayers);
        createInfo.ppEnabledLayerNames = debugLayers;
        createInfo.enabledLayerCount   = debugLayerCount;
    }
#endif

    VkResult result = vkCreateDevice(vulkan->physicalDevice, &createInfo, NULL, &vulkan->device);
    AVD_CHECK_VK_RESULT(result, "Failed to create Vulkan device\n");

    // NOTE: VERY IMPORTANT: This only works as we only use a single vulkan device!
    //                       And thus this optimizes the vulkan calls.
    volkLoadDevice(vulkan->device);

    vulkan->graphicsQueueFamilyIndex    = graphicsQueueFamilyIndex;
    vulkan->computeQueueFamilyIndex     = computeQueueFamilyIndex;
    vulkan->videoDecodeQueueFamilyIndex = videoDecodeQueueFamilyIndex;

    return true;
}

static bool __avdVulkanQueryDeviceProperties(AVD_Vulkan *vulkan)
{
    if (vulkan->supportedFeatures.videoDecode) {
        VkVideoDecodeH264ProfileInfoKHR h264DecodeProfileInfo = {0};
        h264DecodeProfileInfo.sType                           = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR;
        h264DecodeProfileInfo.stdProfileIdc                   = STD_VIDEO_H264_PROFILE_IDC_HIGH;
        h264DecodeProfileInfo.pictureLayout                   = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR;

        VkVideoProfileInfoKHR videoProfileInfo = {0};
        videoProfileInfo.sType                 = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
        videoProfileInfo.videoCodecOperation   = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
        videoProfileInfo.lumaBitDepth          = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
        videoProfileInfo.chromaBitDepth        = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
        videoProfileInfo.chromaSubsampling     = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
        videoProfileInfo.pNext                 = &h264DecodeProfileInfo;

        VkVideoDecodeH264CapabilitiesKHR h264DecodeCapabilities = {0};
        h264DecodeCapabilities.sType                            = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR;
        h264DecodeCapabilities.pNext                            = NULL;

        VkVideoDecodeCapabilitiesKHR decodeCapabilities = {0};
        decodeCapabilities.sType                        = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR;
        decodeCapabilities.pNext                        = &h264DecodeCapabilities;

        VkVideoCapabilitiesKHR videoCapabilities = {0};
        videoCapabilities.sType                  = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
        videoCapabilities.pNext                  = &decodeCapabilities;

        vkGetPhysicalDeviceVideoCapabilitiesKHR(vulkan->physicalDevice, &videoProfileInfo, &videoCapabilities);

        if (decodeCapabilities.flags) {
            vulkan->supportedFeatures.videoDecodeH264BitstreamMinOffsetAlignment = videoCapabilities.minBitstreamBufferOffsetAlignment;
            vulkan->supportedFeatures.videoDecodeH264BitstreamMinSizeAlignment = videoCapabilities.minBitstreamBufferSizeAlignment;
        } else {
            vulkan->supportedFeatures.videoDecode = false;
            AVD_LOG_WARN("Video decode capabilities not supported");
        }
    }

    return true;
}

static bool __avdVulkanGetQueues(AVD_Vulkan *vulkan)
{
    vkGetDeviceQueue(vulkan->device, vulkan->graphicsQueueFamilyIndex, 0, &vulkan->graphicsQueue);
    vkGetDeviceQueue(vulkan->device, vulkan->computeQueueFamilyIndex, 0, &vulkan->computeQueue);
    AVD_CHECK(vulkan->graphicsQueue != VK_NULL_HANDLE && vulkan->computeQueue != VK_NULL_HANDLE);

    if (vulkan->supportedFeatures.videoDecode) {
        vkGetDeviceQueue(vulkan->device, vulkan->videoDecodeQueueFamilyIndex, 0, &vulkan->videoDecodeQueue);
        AVD_CHECK(vulkan->videoDecodeQueue != VK_NULL_HANDLE);
    }
    return true;
}

static bool __avdVulkanCreateCommandPools(AVD_Vulkan *vulkan)
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    poolInfo.queueFamilyIndex = vulkan->graphicsQueueFamilyIndex;
    VkResult result           = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->graphicsCommandPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics command pool\n");

    poolInfo.queueFamilyIndex = vulkan->computeQueueFamilyIndex;
    result                    = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->computeCommandPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create compute command pool\n");

    if (vulkan->supportedFeatures.videoDecode) {
        poolInfo.queueFamilyIndex = vulkan->videoDecodeQueueFamilyIndex;
        result                    = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->videoDecodeCommandPool);
        AVD_CHECK_VK_RESULT(result, "Failed to create video decode command pool\n");
    }

    return true;
}

static bool __avdVulkanDescriptorPoolCreate(AVD_Vulkan *vulkan)
{
    VkDescriptorPoolSize poolSizes[AVD_VULKAN_DESCRIPTOR_TYPE_COUNT] = {0};
    for (int i = 0; i < AVD_VULKAN_DESCRIPTOR_TYPE_COUNT; ++i) {
        poolSizes[i].type            = avdVulkanToVkDescriptorType((AVD_VulkanDescriptorType)i);
        poolSizes[i].descriptorCount = AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE;
    }

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount              = AVD_ARRAY_COUNT(poolSizes);
    poolInfo.pPoolSizes                 = poolSizes;
    poolInfo.maxSets                    = AVD_VULKAN_DESCRIPTOR_TYPE_COUNT * AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE;
    poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    VkResult result = vkCreateDescriptorPool(vulkan->device, &poolInfo, NULL, &vulkan->descriptorPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create descriptor pool\n");

    result = vkCreateDescriptorPool(vulkan->device, &poolInfo, NULL, &vulkan->bindlessDescriptorPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create bindless descriptor pool\n");

    return true;
}

static bool __avdVulkanCreateDescriptorSets(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    VkDescriptorSetLayoutBinding layoutBindings[AVD_VULKAN_DESCRIPTOR_TYPE_COUNT] = {0};
    VkDescriptorBindingFlags layoutBindinFlags[AVD_VULKAN_DESCRIPTOR_TYPE_COUNT]  = {0};

    for (int i = 0; i < AVD_VULKAN_DESCRIPTOR_TYPE_COUNT; ++i) {
        layoutBindings[i].binding            = i;
        layoutBindings[i].descriptorType     = avdVulkanToVkDescriptorType((AVD_VulkanDescriptorType)i);
        layoutBindings[i].descriptorCount    = AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE;
        layoutBindings[i].stageFlags         = VK_SHADER_STAGE_ALL;
        layoutBindings[i].pImmutableSamplers = NULL; // No immutable samplers for now

        layoutBindinFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo = {0};
    bindingFlagsCreateInfo.sType                                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsCreateInfo.bindingCount                                = AVD_VULKAN_DESCRIPTOR_TYPE_COUNT;
    bindingFlagsCreateInfo.pBindingFlags                               = layoutBindinFlags;
    bindingFlagsCreateInfo.pNext                                       = NULL;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {0};
    layoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount                    = AVD_VULKAN_DESCRIPTOR_TYPE_COUNT;
    layoutCreateInfo.pBindings                       = layoutBindings;
    layoutCreateInfo.flags                           = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    layoutCreateInfo.pNext                           = &bindingFlagsCreateInfo;

    VkResult result = vkCreateDescriptorSetLayout(vulkan->device, &layoutCreateInfo, NULL, &vulkan->bindlessDescriptorSetLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create bindless descriptor set layout\n");

    uint32_t descriptorTypeCount = AVD_VULKAN_DESCRIPTOR_TYPE_COUNT * AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE;

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = vulkan->bindlessDescriptorPool;
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = &vulkan->bindlessDescriptorSetLayout;

    result = vkAllocateDescriptorSets(vulkan->device, &allocInfo, &vulkan->bindlessDescriptorSet);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate bindless descriptor set\n");

    return true;
}

bool avdVulkanInit(AVD_Vulkan *vulkan, AVD_Window *window, VkSurfaceKHR *surface)
{
    AVD_CHECK_VK_RESULT(volkInitialize(), "Failed to initialize Vulkan");
    AVD_CHECK(__avdVulkanCreateInstance(vulkan));
    AVD_CHECK(__avdVulkanCreateSurface(vulkan, window->window, surface));
    AVD_CHECK(__avdVulkanPickPhysicalDevice(vulkan));
    AVD_CHECK(__avdVulkanCreateDevice(vulkan, surface));
    AVD_CHECK(__avdVulkanQueryDeviceProperties(vulkan));
    AVD_CHECK(__avdVulkanGetQueues(vulkan));
    AVD_CHECK(__avdVulkanCreateCommandPools(vulkan));
    AVD_CHECK(__avdVulkanDescriptorPoolCreate(vulkan));
    AVD_CHECK(__avdVulkanCreateDescriptorSets(vulkan));
    return true;
}

void avdVulkanWaitIdle(AVD_Vulkan *vulkan)
{
    vkDeviceWaitIdle(vulkan->device);
    vkQueueWaitIdle(vulkan->graphicsQueue);
    vkQueueWaitIdle(vulkan->computeQueue);
}

void avdVulkanDestroySurface(AVD_Vulkan *vulkan, VkSurfaceKHR surface)
{
    vkDestroySurfaceKHR(vulkan->instance, surface, NULL);
}

void avdVulkanShutdown(AVD_Vulkan *vulkan)
{
    vkDeviceWaitIdle(vulkan->device);

    vkDestroyCommandPool(vulkan->device, vulkan->graphicsCommandPool, NULL);
    vkDestroyCommandPool(vulkan->device, vulkan->computeCommandPool, NULL);
    if (vulkan->supportedFeatures.videoDecode) {
        vkDestroyCommandPool(vulkan->device, vulkan->videoDecodeCommandPool, NULL);
    }

    vkDestroyDescriptorPool(vulkan->device, vulkan->descriptorPool, NULL);
    vkDestroyDescriptorPool(vulkan->device, vulkan->bindlessDescriptorPool, NULL);

    vkDestroyDescriptorSetLayout(vulkan->device, vulkan->bindlessDescriptorSetLayout, NULL);

    vkDestroyDevice(vulkan->device, NULL);

#ifdef AVD_DEBUG
    if (vulkan->debugLayersEnabled) {
        vkDestroyDebugUtilsMessengerEXT(vulkan->instance, vulkan->debugMessenger, NULL);
    }
#endif
    vkDestroyInstance(vulkan->instance, NULL);
}

uint32_t avdVulkanFindMemoryType(AVD_Vulkan *vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(vulkan->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    AVD_LOG_ERROR("Failed to find suitable memory type");
    return UINT32_MAX;
}

VkDescriptorType avdVulkanToVkDescriptorType(AVD_VulkanDescriptorType type)
{
    switch (type) {
        case AVD_VULKAN_DESCRIPTOR_TYPE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case AVD_VULKAN_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case AVD_VULKAN_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case AVD_VULKAN_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case AVD_VULKAN_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case AVD_VULKAN_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default:
            AVD_CHECK_MSG(false, "Invalid descriptor type: %d", type);
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}