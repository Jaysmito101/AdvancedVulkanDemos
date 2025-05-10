#include "ps_vulkan.h"
#include "glfw/glfw3.h"

static bool __psVulkanLayersSupported(const char **layers, uint32_t layerCount)
{
    static VkLayerProperties availableLayers[128] = {0};
    uint32_t availableLayerCount = 0;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
    if (availableLayerCount > PS_ARRAY_COUNT(availableLayers))
    {
        PS_LOG("Too many available layers\n");
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
            PS_LOG("Layer %s not supported\n", layers[i]);
            return false;
        }
    }
    return true;
}

static bool __psAddGlfwExtenstions(uint32_t *extensionCount, const char **extensions)
{
    uint32_t count = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    if (glfwExtensions == NULL)
    {
        PS_LOG("Failed to get GLFW extensions\n");
        return false;
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        extensions[i + *extensionCount] = glfwExtensions[i];
    }
    *extensionCount += count;
    return true;
}

#ifdef PS_DEBUG
static bool __psAddDebugUtilsExtenstions(uint32_t *extensionCount, const char **extensions)
{
    extensions[*extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    (*extensionCount)++;
    return true;
}

static bool __psAddDebugLayers(uint32_t *layerCount, const char **layers, bool *debugLayersEnabled)
{
    static const char *debugLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    if (__psVulkanLayersSupported(debugLayers, PS_ARRAY_COUNT(debugLayers)))
    {
        for (uint32_t i = 0; i < PS_ARRAY_COUNT(debugLayers); ++i)
        {
            layers[i + *layerCount] = debugLayers[i];
        }
        *layerCount += PS_ARRAY_COUNT(debugLayers);
        *debugLayersEnabled = true;
    }
    else
    {
        PS_LOG("Debug layers not supported\n");
    }
    return true;
}

static VkBool32 __psDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
    PS_LOG("Debug: %s: %s\n", severity, pCallbackData->pMessage);
    return VK_FALSE; // Don't abort on debug messages
}

static bool __psCreateDebugUtilsMessenger(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = __psDebugUtilsMessengerCallback;
    createInfo.pUserData = vulkan;

    PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessenger =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan->instance, "vkCreateDebugUtilsMessengerEXT");
    PS_CHECK_MSG(createDebugUtilsMessenger != NULL, "Failed to load vkCreateDebugUtilsMessengerEXT");
    VkResult result = createDebugUtilsMessenger(vulkan->instance, &createInfo, NULL, &vulkan->debugMessenger);
    PS_CHECK_VK_RESULT(result, "Failed to create debug utils messenger\n");
    return true;
}

#endif

static bool __psVulkanCreateInstance(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);
    
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Pastel Shadows";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Pastel Shadows";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    uint32_t extensionCount = 0;
    static const char *extensions[64] = {0};

    PS_CHECK(__psAddGlfwExtenstions(&extensionCount, extensions));

#ifdef PS_DEBUG
    PS_CHECK(__psAddDebugUtilsExtenstions(&extensionCount, extensions));
#endif

    uint32_t layerCount = 0;
    static const char *layers[64] = {0};

#ifdef PS_DEBUG
    PS_CHECK(__psAddDebugLayers(&layerCount, layers, &vulkan->debugLayersEnabled));
#endif

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = layers;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, NULL, &vulkan->instance);
    PS_CHECK_VK_RESULT(result, "Failed to create Vulkan instance\n");

#ifdef PS_DEBUG
    if (vulkan->debugLayersEnabled)
    {
        PS_CHECK(__psCreateDebugUtilsMessenger(vulkan));
    }
#endif

    return true;
}

static bool __psVulkanCreateSurface(PS_Vulkan* vulkan, GLFWwindow* window)
{
    VkResult result = glfwCreateWindowSurface(vulkan->instance, window, NULL, &vulkan->swapchain.surface);
    PS_CHECK_VK_RESULT(result, "Failed to create window surface\n");
    return true;
}

static bool __psVulkanPhysicalDeviceCheckExtensions(VkPhysicalDevice device)
{
    static const char *requiredExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    uint32_t extensionCount = 0;
    static VkExtensionProperties extensions[256] = {0};
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
    if (extensionCount == 0)
    {
        PS_LOG("No Vulkan-compatible extensions found\n");
        return false;
    }

    if (extensionCount > PS_ARRAY_COUNT(extensions))
    {
        PS_LOG("Too many Vulkan-compatible extensions found\n");
        return false;
    }
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, extensions);

    for (uint32_t i = 0; i < PS_ARRAY_COUNT(requiredExtensions); ++i)
    {
        bool found = false;
        for (uint32_t j = 0; j < extensionCount; ++j)
        {
            if (strcmp(requiredExtensions[i], extensions[j].extensionName) == 0)
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

static bool __psVulkanPickPhysicalDevice(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkan->instance, &deviceCount, NULL);
    PS_CHECK_MSG(deviceCount > 0, "No Vulkan-compatible devices found\n");
    
    deviceCount = PS_MIN(deviceCount, 64);
    VkPhysicalDevice devices[64] = {0};
    vkEnumeratePhysicalDevices(vulkan->instance, &deviceCount, devices);

    bool foundDiscreteGPU = false;
    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        if (!__psVulkanPhysicalDeviceCheckExtensions(devices[i]))
        {
            PS_LOG("Physical device %s does not support required extensions\n", deviceProperties.deviceName);
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
        PS_LOG("No discrete GPU found, using first available device\n");
        vulkan->physicalDevice = devices[0];
    }

    return true;
}

static int32_t __psVulkanFindQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags flags, VkSurfaceKHR surface, int32_t excludeIndex)
{
    uint32_t queueFamilyCount = 0;
    static VkQueueFamilyProperties queueFamilies[128] = {0};

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    if (queueFamilyCount == 0)
    {
        PS_LOG("No Vulkan-compatible queue families found\n");
        return -1;
    }

    if (queueFamilyCount > PS_ARRAY_COUNT(queueFamilies))
    {
        PS_LOG("Too many Vulkan-compatible queue families found\n");
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

static bool __psVulkanCreateDevice(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    int32_t graphicsQueueFamilyIndex = __psVulkanFindQueueFamilyIndex(vulkan->physicalDevice, VK_QUEUE_GRAPHICS_BIT, vulkan->swapchain.surface, -1);
    PS_CHECK_MSG(graphicsQueueFamilyIndex >= 0, "Failed to find graphics queue family index\n");

    int32_t computeQueueFamilyIndex = __psVulkanFindQueueFamilyIndex(vulkan->physicalDevice, VK_QUEUE_COMPUTE_BIT, VK_NULL_HANDLE, graphicsQueueFamilyIndex);
    PS_CHECK_MSG(computeQueueFamilyIndex >= 0, "Failed to find compute queue family index\n");

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

    static const char *deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    uint32_t deviceExtensionCount = PS_ARRAY_COUNT(deviceExtensions);

    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 2;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.enabledExtensionCount = deviceExtensionCount;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.enabledLayerCount = 0;
    
#ifdef PS_DEBUG
    if (vulkan->debugLayersEnabled)
    {
        static const char *debugLayers[] = {
            "VK_LAYER_KHRONOS_validation",
        };
        uint32_t debugLayerCount = PS_ARRAY_COUNT(debugLayers);
        createInfo.ppEnabledLayerNames = debugLayers;
        createInfo.enabledLayerCount = debugLayerCount;
    }    
#endif

    VkResult result = vkCreateDevice(vulkan->physicalDevice, &createInfo, NULL, &vulkan->device);
    PS_CHECK_VK_RESULT(result, "Failed to create Vulkan device\n");

    vulkan->graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    vulkan->computeQueueFamilyIndex = computeQueueFamilyIndex;

    return true;
}

static bool __psVulkanGetQueues(PS_Vulkan *vulkan)
{
    vkGetDeviceQueue(vulkan->device, vulkan->graphicsQueueFamilyIndex, 0, &vulkan->graphicsQueue);
    vkGetDeviceQueue(vulkan->device, vulkan->computeQueueFamilyIndex, 0, &vulkan->computeQueue);
    PS_CHECK(vulkan->graphicsQueue != VK_NULL_HANDLE && vulkan->computeQueue != VK_NULL_HANDLE);
    return true;
}

static bool __psVulkanCreateCommandPools(PS_Vulkan *vulkan)
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vulkan->graphicsQueueFamilyIndex;

    VkResult result = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->graphicsCommandPool);
    PS_CHECK_VK_RESULT(result, "Failed to create graphics command pool\n");

    poolInfo.queueFamilyIndex = vulkan->computeQueueFamilyIndex;
    result = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->computeCommandPool);
    PS_CHECK_VK_RESULT(result, "Failed to create compute command pool\n");

    return true;
}   

static bool __psVulkanDescriptorPoolCreate(PS_Vulkan *vulkan)
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
    uint32_t poolSizeCount = PS_ARRAY_COUNT(poolSizes);

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizeCount;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 10000;

    VkResult result = vkCreateDescriptorPool(vulkan->device, &poolInfo, NULL, &vulkan->descriptorPool);
    PS_CHECK_VK_RESULT(result, "Failed to create descriptor pool\n");

    return true;
}

bool psVulkanInit(PS_Vulkan *vulkan, PS_Window* window)
{
    vulkan->swapchain.swapchainReady = false;

    PS_CHECK(__psVulkanCreateInstance(vulkan));
    PS_CHECK(__psVulkanCreateSurface(vulkan, window->window));
    PS_CHECK(__psVulkanPickPhysicalDevice(vulkan));
    PS_CHECK(__psVulkanCreateDevice(vulkan));
    PS_CHECK(__psVulkanGetQueues(vulkan));
    PS_CHECK(__psVulkanCreateCommandPools(vulkan));
    PS_CHECK(__psVulkanDescriptorPoolCreate(vulkan));
    PS_CHECK(psVulkanSwapchainCreate(vulkan, window));
    PS_CHECK(psVulkanRendererCreate(vulkan));
    PS_CHECK(psVulkanPresentationInit(vulkan));
    return true;
}

void psVulkanShutdown(PS_Vulkan *vulkan)
{
    vkDeviceWaitIdle(vulkan->device);
    psVulkanPresentationDestroy(vulkan);
    psVulkanRendererDestroy(vulkan);

    vkDestroyCommandPool(vulkan->device, vulkan->graphicsCommandPool, NULL);
    vkDestroyCommandPool(vulkan->device, vulkan->computeCommandPool, NULL);
    psVulkanSwapchainDestroy(vulkan);

    vkDestroyDescriptorPool(vulkan->device, vulkan->descriptorPool, NULL);
    vkDestroyDevice(vulkan->device, NULL);


#ifdef PS_DEBUG
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

    vkDestroySurfaceKHR(vulkan->instance, vulkan->swapchain.surface, NULL);
    vkDestroyInstance(vulkan->instance, NULL);
}

uint32_t psVulkanFindMemoryType(PS_Vulkan* vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(vulkan->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    PS_LOG("Failed to find suitable memory type\n");
    return UINT32_MAX;
}