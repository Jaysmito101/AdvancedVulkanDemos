#include "core/avd_base.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"
#include <stdint.h>

static AVD_Vulkan *__avdGlobalVulkanInstance = NULL;

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
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
};

static const char *__avd_VulkanVideoDecodeExtensions[] = {
    VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME,
    VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME,
    // VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME, // we only deal with h264 for now!
};

static const char *__avd_VulkanVideoEncodeExtensions[] = {
    VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME,
    VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME,
    // VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME, // we only deal with h264 for now!
};

static AVD_VulkanFeatures *__avdVulkanFeaturesInit(AVD_VulkanFeatures *features)
{
    AVD_ASSERT(features != NULL);
    features->rayTracing  = false;
    features->videoCore   = false;
    features->videoDecode = false;
    features->videoEncode = false;
    return features;
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
    if (vulkan->supportedFeatures.videoCore) {
        __avdVulkanAddDeviceExtensionsToList(deviceExtensions, &count, __avd_VulkanVideoExtensions, AVD_ARRAY_COUNT(__avd_VulkanVideoExtensions));
        if (vulkan->supportedFeatures.videoDecode) {
            __avdVulkanAddDeviceExtensionsToList(deviceExtensions, &count, __avd_VulkanVideoDecodeExtensions, AVD_ARRAY_COUNT(__avd_VulkanVideoDecodeExtensions));
        }
        if (vulkan->supportedFeatures.videoEncode) {
            __avdVulkanAddDeviceExtensionsToList(deviceExtensions, &count, __avd_VulkanVideoEncodeExtensions, AVD_ARRAY_COUNT(__avd_VulkanVideoEncodeExtensions));
        }
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

static bool __avdVulkanCreateInstance(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "AVD",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "AVD",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_4,
    };

    uint32_t extensionCount           = 0;
    static const char *extensions[64] = {0};

    AVD_CHECK(__avdAddGlfwExtenstions(&extensionCount, extensions));
    AVD_CHECK(__avdAddSurfaceExtensions(&extensionCount, extensions));

    AVD_DEBUG_ONLY(avdVulkanAddDebugUtilsExtensions(&extensionCount, extensions));

    uint32_t layerCount           = 0;
    static const char *layers[64] = {0};

    AVD_DEBUG_ONLY(avdVulkanAddDebugLayers(&layerCount, layers, &vulkan->debugger.layersEnabled));

    VkInstanceCreateInfo createInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = layerCount,
        .ppEnabledLayerNames     = layers,
        .enabledExtensionCount   = extensionCount,
        .ppEnabledExtensionNames = extensions,
    };

    VkResult result = vkCreateInstance(&createInfo, NULL, &vulkan->instance);
    AVD_CHECK_VK_RESULT(result, "Failed to create Vulkan instance\n");

    volkLoadInstance(vulkan->instance);

    return true;
}

static bool __avdVulkanCreateSurface(AVD_Vulkan *vulkan, GLFWwindow *window, VkSurfaceKHR *surface)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(window != NULL);
    AVD_ASSERT(surface != NULL);

    AVD_CHECK_VK_RESULT(
        glfwCreateWindowSurface(
            vulkan->instance,
            window,
            NULL,
            surface),
        "Failed to create window surface\n");

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
    outFeatures->videoCore   = __avdVulkanPhysicalDeviceCheckExtensionsSet(extensions, extensionCount, __avd_VulkanVideoExtensions, AVD_ARRAY_COUNT(__avd_VulkanVideoExtensions));
    outFeatures->videoDecode = __avdVulkanPhysicalDeviceCheckExtensionsSet(extensions, extensionCount, __avd_VulkanVideoExtensions, AVD_ARRAY_COUNT(__avd_VulkanVideoDecodeExtensions));
    outFeatures->videoEncode = __avdVulkanPhysicalDeviceCheckExtensionsSet(extensions, extensionCount, __avd_VulkanVideoEncodeExtensions, AVD_ARRAY_COUNT(__avd_VulkanVideoEncodeExtensions));

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
    AVD_LOG_INFO("Selected GPU device: %s", deviceProperties.deviceName);

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

    int32_t videoEncodeQueueFamilyIndex = __avdVulkanFindQueueFamilyIndex(vulkan->physicalDevice, VK_QUEUE_VIDEO_ENCODE_BIT_KHR | VK_QUEUE_TRANSFER_BIT, VK_NULL_HANDLE, -1);
    if (vulkan->supportedFeatures.videoEncode) {
        AVD_CHECK_MSG(videoEncodeQueueFamilyIndex >= 0, "Failed to find video encode queue family index\n");
    }

    // We only need one graphics queue and one compute queue for now
    VkDeviceQueueCreateInfo queueCreateInfos[4] = {0};
    AVD_Size queueCreateInfoCount               = 0;
    float queuePriority                         = 1.0f;

    // first the graphics queue
    queueCreateInfos[queueCreateInfoCount] = (VkDeviceQueueCreateInfo){
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority,
    };
    queueCreateInfoCount++;

    // then the compute queue
    queueCreateInfos[queueCreateInfoCount] = (VkDeviceQueueCreateInfo){
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = computeQueueFamilyIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority,
    };
    queueCreateInfoCount++;

    if (vulkan->supportedFeatures.videoDecode) {
        // finally the video decode queue
        queueCreateInfos[queueCreateInfoCount] = (VkDeviceQueueCreateInfo){
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = videoDecodeQueueFamilyIndex,
            .queueCount       = 1,
            .pQueuePriorities = &queuePriority,
        };
        queueCreateInfoCount++;
    }

    if (vulkan->supportedFeatures.videoEncode) {
        // finally the video encode queue
        queueCreateInfos[queueCreateInfoCount] = (VkDeviceQueueCreateInfo){
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = videoEncodeQueueFamilyIndex,
            .queueCount       = 1,
            .pQueuePriorities = &queuePriority,
        };
        queueCreateInfoCount++;
    }

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        // .rayTracingPipeline = VK_TRUE,
        .pNext = NULL,
    };

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {
        .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
        .rayQuery = VK_TRUE,
        .pNext    = NULL,
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {
        .sType                                                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure                                 = VK_TRUE,
        .descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE,
        .pNext                                                 = &rayQueryFeatures,
    };

    VkPhysicalDeviceVulkan12Features deviceVulkan12Features = {
        .sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing                            = VK_TRUE,
        .bufferDeviceAddress                           = VK_TRUE,
        .shaderFloat16                                 = VK_TRUE,
        .shaderInt8                                    = VK_TRUE,
        .uniformAndStorageBuffer8BitAccess             = VK_TRUE,
        .storageBuffer8BitAccess                       = VK_TRUE,
        .drawIndirectCount                             = VK_TRUE,
        .imagelessFramebuffer                          = VK_TRUE,
        .runtimeDescriptorArray                        = VK_TRUE,
        .descriptorBindingPartiallyBound               = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE,
        .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingUpdateUnusedWhilePending     = VK_TRUE,
        .descriptorBindingVariableDescriptorCount      = VK_TRUE,
        .pNext                                         = &accelerationStructureFeatures,
    };

    VkPhysicalDeviceVulkan11Features deviceVulkan11Features = {
        .sType                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .uniformAndStorageBuffer16BitAccess = VK_TRUE,
        .storageBuffer16BitAccess           = VK_TRUE,
        .pNext                              = &deviceVulkan12Features,
    };

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1Features = {
        .sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT,
        .swapchainMaintenance1 = VK_TRUE,
        .pNext                 = &deviceVulkan11Features,
    };

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {
        .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .features = (VkPhysicalDeviceFeatures){
            .multiDrawIndirect       = VK_TRUE,
            .pipelineStatisticsQuery = VK_TRUE,
            .samplerAnisotropy       = VK_TRUE,
            .shaderInt64             = VK_TRUE,
            .shaderInt16             = VK_TRUE,
        },
        .pNext = &swapchainMaintenance1Features,
    };

    uint32_t deviceExtensionCount = 0;
    const char **deviceExtensions = __avdGetVulkanDeviceExtensions(vulkan, &deviceExtensionCount);

    VkDeviceCreateInfo createInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = (AVD_UInt32)queueCreateInfoCount,
        .pQueueCreateInfos       = queueCreateInfos,
        .enabledExtensionCount   = deviceExtensionCount,
        .ppEnabledExtensionNames = deviceExtensions,
        .pNext                   = &deviceFeatures2,
    };

#ifdef AVD_DEBUG
    // TODO: Maybe move this to the debugger creation?
    if (vulkan->debugger.layersEnabled) {
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

    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_DEVICE, vulkan->device, "Core/Device");

    vulkan->graphicsQueueFamilyIndex    = graphicsQueueFamilyIndex;
    vulkan->computeQueueFamilyIndex     = computeQueueFamilyIndex;
    vulkan->videoDecodeQueueFamilyIndex = videoDecodeQueueFamilyIndex;
    vulkan->videoEncodeQueueFamilyIndex = videoEncodeQueueFamilyIndex;

    return true;
}

static bool __avdVulkanQueryDeviceProperties(AVD_Vulkan *vulkan)
{
    if (vulkan->supportedFeatures.videoDecode) {
        VkVideoDecodeH264ProfileInfoKHR h264DecodeProfileInfo = {
            .sType         = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR,
            .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH,
            .pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR,
            .pNext         = NULL,
        };

        VkVideoProfileInfoKHR videoProfileInfo = {
            .sType               = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
            .videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR,
            .lumaBitDepth        = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
            .chromaBitDepth      = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
            .chromaSubsampling   = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
            .pNext               = &h264DecodeProfileInfo,
        };

        vulkan->supportedFeatures.videoDecodeH264Capabilities = (VkVideoDecodeH264CapabilitiesKHR){
            .sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR,
            .pNext = NULL,
        };

        vulkan->supportedFeatures.videoDecodeCapabilities = (VkVideoDecodeCapabilitiesKHR){
            .sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR,
            .pNext = &vulkan->supportedFeatures.videoDecodeH264Capabilities,
        };

        vulkan->supportedFeatures.videoCapabilitiesDecode = (VkVideoCapabilitiesKHR){
            .sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR,
            .pNext = &vulkan->supportedFeatures.videoDecodeCapabilities,
        };

        vkGetPhysicalDeviceVideoCapabilitiesKHR(vulkan->physicalDevice, &videoProfileInfo, &vulkan->supportedFeatures.videoCapabilitiesDecode);
    }

    if (vulkan->supportedFeatures.videoEncode) {
        VkVideoEncodeH264ProfileInfoKHR h264EncodeProfileInfo = {
            .sType         = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR,
            .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH,
            .pNext         = NULL,
        };

        VkVideoProfileInfoKHR videoProfileInfo = {
            .sType               = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
            .videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR,
            .lumaBitDepth        = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
            .chromaBitDepth      = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
            .chromaSubsampling   = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
            .pNext               = &h264EncodeProfileInfo,
        };

        vulkan->supportedFeatures.videoEncodeH264Capabilities = (VkVideoEncodeH264CapabilitiesKHR){
            .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_KHR,
            .pNext = NULL,
        };

        vulkan->supportedFeatures.videoEncodeCapabilities = (VkVideoEncodeCapabilitiesKHR){
            .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR,
            .pNext = &vulkan->supportedFeatures.videoEncodeH264Capabilities,
        };

        vulkan->supportedFeatures.videoCapabilitiesEncode = (VkVideoCapabilitiesKHR){
            .sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR,
            .pNext = &vulkan->supportedFeatures.videoEncodeCapabilities,
        };

        vkGetPhysicalDeviceVideoCapabilitiesKHR(vulkan->physicalDevice, &videoProfileInfo, &vulkan->supportedFeatures.videoCapabilitiesEncode);
    }

    return true;
}

static bool __avdVulkanGetQueues(AVD_Vulkan *vulkan)
{
    vkGetDeviceQueue(vulkan->device, vulkan->graphicsQueueFamilyIndex, 0, &vulkan->graphicsQueue);
    AVD_CHECK_MSG(vulkan->graphicsQueue != VK_NULL_HANDLE, "Failed to get graphics queue\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_QUEUE,
        (uint64_t)vulkan->graphicsQueue,
        "Core/GraphicsQueue");

    vkGetDeviceQueue(vulkan->device, vulkan->computeQueueFamilyIndex, 0, &vulkan->computeQueue);
    AVD_CHECK_MSG(vulkan->computeQueue != VK_NULL_HANDLE, "Failed to get compute queue\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_QUEUE,
        (uint64_t)vulkan->computeQueue,
        "Core/ComputeQueue");

    if (vulkan->supportedFeatures.videoDecode) {
        vkGetDeviceQueue(vulkan->device, vulkan->videoDecodeQueueFamilyIndex, 0, &vulkan->videoDecodeQueue);
        AVD_CHECK(vulkan->videoDecodeQueue != VK_NULL_HANDLE);
        AVD_DEBUG_VK_SET_OBJECT_NAME(
            VK_OBJECT_TYPE_QUEUE,
            (uint64_t)vulkan->videoDecodeQueue,
            "Core/VideoDecodeQueue");
    }

    if (vulkan->supportedFeatures.videoEncode) {
        vkGetDeviceQueue(vulkan->device, vulkan->videoEncodeQueueFamilyIndex, 0, &vulkan->videoEncodeQueue);
        AVD_CHECK(vulkan->videoEncodeQueue != VK_NULL_HANDLE);
        AVD_DEBUG_VK_SET_OBJECT_NAME(
            VK_OBJECT_TYPE_QUEUE,
            (uint64_t)vulkan->videoEncodeQueue,
            "Core/VideoEncodeQueue");
    }

    return true;
}

static bool __avdVulkanCreateCommandPools(AVD_Vulkan *vulkan)
{
    VkCommandPoolCreateInfo poolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vulkan->graphicsQueueFamilyIndex,
    };

    poolInfo.queueFamilyIndex = vulkan->graphicsQueueFamilyIndex;
    VkResult result           = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->graphicsCommandPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics command pool\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_COMMAND_POOL,
        (uint64_t)vulkan->graphicsCommandPool,
        "Core/GraphicsCommandPool");

    poolInfo.queueFamilyIndex = vulkan->computeQueueFamilyIndex;
    result                    = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->computeCommandPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create compute command pool\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_COMMAND_POOL,
        (uint64_t)vulkan->computeCommandPool,
        "Core/ComputeCommandPool");

    if (vulkan->supportedFeatures.videoDecode) {
        poolInfo.queueFamilyIndex = vulkan->videoDecodeQueueFamilyIndex;
        result                    = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->videoDecodeCommandPool);
        AVD_CHECK_VK_RESULT(result, "Failed to create video decode command pool\n");
        AVD_DEBUG_VK_SET_OBJECT_NAME(
            VK_OBJECT_TYPE_COMMAND_POOL,
            (uint64_t)vulkan->videoDecodeCommandPool,
            "Core/VideoDecodeCommandPool");
    }

    if (vulkan->supportedFeatures.videoEncode) {
        poolInfo.queueFamilyIndex = vulkan->videoEncodeQueueFamilyIndex;
        result                    = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->videoEncodeCommandPool);
        AVD_CHECK_VK_RESULT(result, "Failed to create video encode command pool\n");
        AVD_DEBUG_VK_SET_OBJECT_NAME(
            VK_OBJECT_TYPE_COMMAND_POOL,
            (uint64_t)vulkan->videoEncodeCommandPool,
            "Core/VideoEncodeCommandPool");
    }

    return true;
}

static bool __avdVulkanDescriptorPoolCreate(AVD_Vulkan *vulkan)
{
    VkDescriptorPoolSize poolSizes[AVD_VULKAN_DESCRIPTOR_TYPE_COUNT] = {0};
    for (int i = 0; i < AVD_VULKAN_DESCRIPTOR_TYPE_COUNT; ++i) {
        poolSizes[i] = (VkDescriptorPoolSize){
            .type            = avdVulkanToVkDescriptorType((AVD_VulkanDescriptorType)i),
            .descriptorCount = AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE,
        };
    }

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = AVD_ARRAY_COUNT(poolSizes),
        .pPoolSizes    = poolSizes,
        .maxSets       = AVD_VULKAN_DESCRIPTOR_TYPE_COUNT * AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
    };

    VkResult result = vkCreateDescriptorPool(vulkan->device, &poolInfo, NULL, &vulkan->descriptorPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create descriptor pool\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_DESCRIPTOR_POOL,
        (uint64_t)vulkan->descriptorPool,
        "Core/DescriptorPool");

    result = vkCreateDescriptorPool(vulkan->device, &poolInfo, NULL, &vulkan->bindlessDescriptorPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create bindless descriptor pool\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_DESCRIPTOR_POOL,
        (uint64_t)vulkan->bindlessDescriptorPool,
        "Core/BindlessDescriptorPool");

    return true;
}

static bool __avdVulkanCreateDescriptorSets(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    VkDescriptorSetLayoutBinding layoutBindings[AVD_VULKAN_DESCRIPTOR_TYPE_COUNT] = {0};
    VkDescriptorBindingFlags layoutBindinFlags[AVD_VULKAN_DESCRIPTOR_TYPE_COUNT]  = {0};

    for (int i = 0; i < AVD_VULKAN_DESCRIPTOR_TYPE_COUNT; ++i) {
        layoutBindings[i] = (VkDescriptorSetLayoutBinding){
            .binding            = i,
            .descriptorType     = avdVulkanToVkDescriptorType((AVD_VulkanDescriptorType)i),
            .descriptorCount    = AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE,
            .stageFlags         = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = NULL, // No immutable samplers for now
        };

        layoutBindinFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsCreateInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
        .bindingCount  = AVD_VULKAN_DESCRIPTOR_TYPE_COUNT,
        .pBindingFlags = layoutBindinFlags,
        .pNext         = NULL,
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = AVD_VULKAN_DESCRIPTOR_TYPE_COUNT,
        .pBindings    = layoutBindings,
        .flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
        .pNext        = &bindingFlagsCreateInfo,
    };

    VkResult result = vkCreateDescriptorSetLayout(vulkan->device, &layoutCreateInfo, NULL, &vulkan->bindlessDescriptorSetLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create bindless descriptor set layout\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        (uint64_t)vulkan->bindlessDescriptorSetLayout,
        "Core/BindlessDescriptorSetLayout");

    uint32_t descriptorTypeCount = AVD_VULKAN_DESCRIPTOR_TYPE_COUNT * AVD_VULKAN_DESCRIPTOR_COUNT_PER_TYPE;

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = vulkan->bindlessDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &vulkan->bindlessDescriptorSetLayout,
    };

    result = vkAllocateDescriptorSets(vulkan->device, &allocInfo, &vulkan->bindlessDescriptorSet);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate bindless descriptor set\n");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_DESCRIPTOR_SET,
        (uint64_t)vulkan->bindlessDescriptorSet,
        "Core/BindlessDescriptorSet");

    return true;
}

bool avdVulkanInstanceLayersSupported(const char **layers, uint32_t layerCount)
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

AVD_Vulkan *avdVulkanGetGlobalInstance()
{
    return __avdGlobalVulkanInstance;
}

bool avdVulkanInit(AVD_Vulkan *vulkan, AVD_Window *window, VkSurfaceKHR *surface)
{
    AVD_CHECK_MSG(__avdGlobalVulkanInstance == NULL, "Vulkan instance already initialized");
    __avdGlobalVulkanInstance = vulkan;

    AVD_CHECK_VK_RESULT(volkInitialize(), "Failed to initialize Vulkan");
    AVD_CHECK(__avdVulkanCreateInstance(vulkan));
    AVD_DEBUG_ONLY(AVD_CHECK(avdVulkanDebuggerCreate(vulkan)));
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
    if (vulkan->supportedFeatures.videoEncode) {
        vkDestroyCommandPool(vulkan->device, vulkan->videoEncodeCommandPool, NULL);
    }

    vkDestroyDescriptorPool(vulkan->device, vulkan->descriptorPool, NULL);
    vkDestroyDescriptorPool(vulkan->device, vulkan->bindlessDescriptorPool, NULL);

    vkDestroyDescriptorSetLayout(vulkan->device, vulkan->bindlessDescriptorSetLayout, NULL);

    vkDestroyDevice(vulkan->device, NULL);

    AVD_DEBUG_ONLY(avdVulkanDebuggerDestroy(vulkan));
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
