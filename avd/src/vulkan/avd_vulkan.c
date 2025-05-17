#include "vulkan/avd_vulkan_base.h"    

static const char *__avd_RequiredVulkanExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
};

static bool __avdVulkanLayersSupported(const char **layers, uint32_t layerCount)
{
    static VkLayerProperties availableLayers[128] = {0};
    uint32_t availableLayerCount = 0;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
    if (availableLayerCount > AVD_ARRAY_COUNT(availableLayers))
    {
        AVD_LOG("Too many available layers\n");
        return false;
    }
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);
    for (uint32_t i = 0; i < layerCount; ++i)
    {
        bool found = false;
        for (uint32_t j = 0; j < availableLayerCount; ++j)
        {
            if (strcmp(layers[i], availableLayers[j].layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            AVD_LOG("Layer %s not supported\n", layers[i]);
            return false;
        }
    }
    return true;
}

static bool __avdAddGlfwExtenstions(uint32_t *extensionCount, const char **extensions)
{
    uint32_t count = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    if (glfwExtensions == NULL)
    {
        AVD_LOG("Failed to get GLFW extensions\n");
        return false;
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        extensions[i + *extensionCount] = glfwExtensions[i];
    }
    *extensionCount += count;
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

    if (__avdVulkanLayersSupported(debugLayers, AVD_ARRAY_COUNT(debugLayers)))
    {
        for (uint32_t i = 0; i < AVD_ARRAY_COUNT(debugLayers); ++i)
        {
            layers[i + *layerCount] = debugLayers[i];
        }
        *layerCount += AVD_ARRAY_COUNT(debugLayers);
        *debugLayersEnabled = true;
    }
    else
    {
        AVD_LOG("Debug layers not supported\n");
    }
    return true;
}

static VkBool32 __avdDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                void *pUserData)
{
    const char *severity = NULL;
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        severity = "VERBOSE";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        severity = "INFO";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        severity = "WARNING";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        severity = "ERROR";
        break;
    default:
        severity = "UNKNOWN";
        break;
    }
    AVD_LOG("Debug: %s: %s\n", severity, pCallbackData->pMessage);
    return VK_FALSE; // Don't abort on debug messages
}

static bool __avdCreateDebugUtilsMessenger(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = __avdDebugUtilsMessengerCallback;
    createInfo.pUserData = vulkan;

    PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessenger =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan->instance, "vkCreateDebugUtilsMessengerEXT");
    AVD_CHECK_MSG(createDebugUtilsMessenger != NULL, "Failed to load vkCreateDebugUtilsMessengerEXT");
    VkResult result = createDebugUtilsMessenger(vulkan->instance, &createInfo, NULL, &vulkan->debugMessenger);
    AVD_CHECK_VK_RESULT(result, "Failed to create debug utils messenger\n");
    return true;
}

#endif

static bool __avdVulkanCreateInstance(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Pastel Shadows";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Pastel Shadows";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    uint32_t extensionCount = 0;
    static const char *extensions[64] = {0};

    AVD_CHECK(__avdAddGlfwExtenstions(&extensionCount, extensions));

#ifdef AVD_DEBUG
    AVD_CHECK(__avdAddDebugUtilsExtenstions(&extensionCount, extensions));
#endif

    uint32_t layerCount = 0;
    static const char *layers[64] = {0};

#ifdef AVD_DEBUG
    AVD_CHECK(__avdAddDebugLayers(&layerCount, layers, &vulkan->debugLayersEnabled));
#endif

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = layers;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, NULL, &vulkan->instance);
    AVD_CHECK_VK_RESULT(result, "Failed to create Vulkan instance\n");

#ifdef AVD_DEBUG
    if (vulkan->debugLayersEnabled)
    {
        AVD_CHECK(__avdCreateDebugUtilsMessenger(vulkan));
    }
#endif

    return true;
}

static bool __avdVulkanCreateSurface(AVD_Vulkan* vulkan, GLFWwindow* window, VkSurfaceKHR* surface)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(window != NULL);
    AVD_ASSERT(surface != NULL);

    VkResult result = glfwCreateWindowSurface(vulkan->instance, window, NULL, surface);
    AVD_CHECK_VK_RESULT(result, "Failed to create window surface\n");
    return true;
}

static bool __avdVulkanPhysicalDeviceCheckExtensions(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;
    static VkExtensionProperties extensions[256] = {0};
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
    if (extensionCount == 0)
    {
        AVD_LOG("No Vulkan-compatible extensions found\n");
        return false;
    }
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, extensions);

    for (uint32_t i = 0; i < AVD_ARRAY_COUNT(__avd_RequiredVulkanExtensions); ++i)
    {
        bool found = false;
        for (uint32_t j = 0; j < extensionCount; ++j)
        {
            if (strcmp(__avd_RequiredVulkanExtensions[i], extensions[j].extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }
    }
    return true;
}

static bool __avdVulkanPickPhysicalDevice(AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkan->instance, &deviceCount, NULL);
    AVD_CHECK_MSG(deviceCount > 0, "No Vulkan-compatible devices found\n");
    
    deviceCount = AVD_MIN(deviceCount, 64);
    VkPhysicalDevice devices[64] = {0};
    vkEnumeratePhysicalDevices(vulkan->instance, &deviceCount, devices);

    bool foundDiscreteGPU = false;
    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        if (!__avdVulkanPhysicalDeviceCheckExtensions(devices[i]))
        {
            AVD_LOG("Physical device %s does not support required extensions\n", deviceProperties.deviceName);
            continue;
        }

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            vulkan->physicalDevice = devices[i];
            foundDiscreteGPU = true;
            break;
        }
    }

    if (!foundDiscreteGPU)
    {
        AVD_LOG("No discrete GPU found, using first available device\n");
        vulkan->physicalDevice = devices[0];
    }

    VkPhysicalDeviceProperties deviceProperties = {0};
    vkGetPhysicalDeviceProperties(vulkan->physicalDevice, &deviceProperties);
    AVD_LOG("Selected physical device: %s\n", deviceProperties.deviceName);

    return true;
}

static int32_t __avdVulkanFindQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags flags, VkSurfaceKHR surface, int32_t excludeIndex)
{
    uint32_t queueFamilyCount = 0;
    static VkQueueFamilyProperties queueFamilies[128] = {0};

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    if (queueFamilyCount == 0)
    {
        AVD_LOG("No Vulkan-compatible queue families found\n");
        return -1;
    }

    if (queueFamilyCount > AVD_ARRAY_COUNT(queueFamilies))
    {
        AVD_LOG("Too many Vulkan-compatible queue families found\n");
        return -1;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (i == excludeIndex)
        {
            continue;
        }
        if ((queueFamilies[i].queueFlags & flags) == flags)
        {
            if (surface != VK_NULL_HANDLE)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (!presentSupport)
                {
                    continue;
                }
            }
            return i;
        }
    }
    return -1;
}

static bool __avdVulkanCreateDevice(AVD_Vulkan *vulkan, VkSurfaceKHR* surface)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(surface != NULL);

    int32_t graphicsQueueFamilyIndex = __avdVulkanFindQueueFamilyIndex(vulkan->physicalDevice, VK_QUEUE_GRAPHICS_BIT, *surface, -1);
    AVD_CHECK_MSG(graphicsQueueFamilyIndex >= 0, "Failed to find graphics queue family index\n");

    int32_t computeQueueFamilyIndex = __avdVulkanFindQueueFamilyIndex(vulkan->physicalDevice, VK_QUEUE_COMPUTE_BIT, VK_NULL_HANDLE, graphicsQueueFamilyIndex);
    AVD_CHECK_MSG(computeQueueFamilyIndex >= 0, "Failed to find compute queue family index\n");

    // We only need one graphics queue and one compute queue for now
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {0};
    float queuePriority = 1.0f;

    //  first the graphics queue
    queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfos[0].queueCount = 1;
    queueCreateInfos[0].pQueuePriorities = &queuePriority;

    // then the compute queue
    queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[1].queueFamilyIndex = computeQueueFamilyIndex;
    queueCreateInfos[1].queueCount = 1;
    queueCreateInfos[1].pQueuePriorities = &queuePriority;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {0};
    rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    rayTracingPipelineFeatures.pNext = NULL;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {0};
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery = VK_TRUE;
    rayQueryFeatures.pNext = &rayTracingPipelineFeatures;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {0};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;
    accelerationStructureFeatures.pNext = &rayQueryFeatures;

    VkPhysicalDeviceVulkan12Features deviceVulkan12Features = {0};
    deviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    deviceVulkan12Features.descriptorIndexing = VK_TRUE;
    deviceVulkan12Features.bufferDeviceAddress = VK_TRUE;
    deviceVulkan12Features.shaderFloat16 = VK_TRUE;
    deviceVulkan12Features.shaderInt8 = VK_TRUE;
    deviceVulkan12Features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
    deviceVulkan12Features.storageBuffer8BitAccess = VK_TRUE;
    deviceVulkan12Features.drawIndirectCount = VK_TRUE;
    deviceVulkan12Features.pNext = &accelerationStructureFeatures;

    VkPhysicalDeviceVulkan11Features deviceVulkan11Features = {0};
    deviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    deviceVulkan11Features.uniformAndStorageBuffer16BitAccess = VK_TRUE;
    deviceVulkan11Features.storageBuffer16BitAccess = VK_TRUE;
    deviceVulkan11Features.pNext = &deviceVulkan12Features;

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {0};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features.multiDrawIndirect = VK_TRUE;
    deviceFeatures2.features.pipelineStatisticsQuery = VK_TRUE;
    deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
    deviceFeatures2.features.shaderInt64 = VK_TRUE;
    deviceFeatures2.features.shaderInt16 = VK_TRUE;
    deviceFeatures2.pNext = &deviceVulkan11Features;

    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 2;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.enabledExtensionCount = AVD_ARRAY_COUNT(__avd_RequiredVulkanExtensions);
    createInfo.ppEnabledExtensionNames = __avd_RequiredVulkanExtensions;
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = &deviceFeatures2;
    
#ifdef AVD_DEBUG
    if (vulkan->debugLayersEnabled)
    {
        static const char *debugLayers[] = {
            "VK_LAYER_KHRONOS_validation",
        };
        uint32_t debugLayerCount = AVD_ARRAY_COUNT(debugLayers);
        createInfo.ppEnabledLayerNames = debugLayers;
        createInfo.enabledLayerCount = debugLayerCount;
    }    
#endif

    VkResult result = vkCreateDevice(vulkan->physicalDevice, &createInfo, NULL, &vulkan->device);
    AVD_CHECK_VK_RESULT(result, "Failed to create Vulkan device\n");

    vulkan->graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    vulkan->computeQueueFamilyIndex = computeQueueFamilyIndex;

    return true;
}

static bool __avdVulkanGetQueues(AVD_Vulkan *vulkan)
{
    vkGetDeviceQueue(vulkan->device, vulkan->graphicsQueueFamilyIndex, 0, &vulkan->graphicsQueue);
    vkGetDeviceQueue(vulkan->device, vulkan->computeQueueFamilyIndex, 0, &vulkan->computeQueue);
    AVD_CHECK(vulkan->graphicsQueue != VK_NULL_HANDLE && vulkan->computeQueue != VK_NULL_HANDLE);
    return true;
}

static bool __avdVulkanCreateCommandPools(AVD_Vulkan *vulkan)
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vulkan->graphicsQueueFamilyIndex;

    VkResult result = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->graphicsCommandPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics command pool\n");

    poolInfo.queueFamilyIndex = vulkan->computeQueueFamilyIndex;
    result = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->computeCommandPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create compute command pool\n");

    return true;
}   

static bool __avdVulkanDescriptorPoolCreate(AVD_Vulkan *vulkan)
{
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 100},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10},
    };
    uint32_t poolSizeCount = AVD_ARRAY_COUNT(poolSizes);

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizeCount;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 10000;

    VkResult result = vkCreateDescriptorPool(vulkan->device, &poolInfo, NULL, &vulkan->descriptorPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create descriptor pool\n");

    return true;
}

bool avdVulkanInit(AVD_Vulkan *vulkan, AVD_Window* window, VkSurfaceKHR* surface)
{
    AVD_CHECK(__avdVulkanCreateInstance(vulkan));
    AVD_CHECK(__avdVulkanCreateSurface(vulkan, window->window, surface));
    AVD_CHECK(__avdVulkanPickPhysicalDevice(vulkan));
    AVD_CHECK(__avdVulkanCreateDevice(vulkan, surface));
    AVD_CHECK(__avdVulkanGetQueues(vulkan));
    AVD_CHECK(__avdVulkanCreateCommandPools(vulkan));
    AVD_CHECK(__avdVulkanDescriptorPoolCreate(vulkan));
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

    vkDestroyDescriptorPool(vulkan->device, vulkan->descriptorPool, NULL);
    vkDestroyDevice(vulkan->device, NULL);


#ifdef AVD_DEBUG
    if (vulkan->debugLayersEnabled)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebugUtilsMessenger != NULL)
        {
            destroyDebugUtilsMessenger(vulkan->instance, vulkan->debugMessenger, NULL);
        }
    }
#endif
    vkDestroyInstance(vulkan->instance, NULL);
}

uint32_t avdVulkanFindMemoryType(AVD_Vulkan* vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(vulkan->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    AVD_LOG("Failed to find suitable memory type\n");
    return UINT32_MAX;
}