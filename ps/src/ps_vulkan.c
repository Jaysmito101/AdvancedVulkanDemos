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

static bool __psCreateDebugUtilsMessenger(PS_GameState *gameState)
{
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
    createInfo.pUserData = gameState;

    PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessenger =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(gameState->vulkan.instance, "vkCreateDebugUtilsMessengerEXT");
    if (createDebugUtilsMessenger == NULL)
    {
        PS_LOG("Failed to load vkCreateDebugUtilsMessengerEXT\n");
        return false;
    }
    VkResult result = createDebugUtilsMessenger(gameState->vulkan.instance, &createInfo, NULL, &gameState->vulkan.debugMessenger);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create debug utils messenger\n");
        return false;
    }
    return true;
}

#endif

static bool __psVulkanCreateInstance(PS_GameState *gameState)
{
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Pastel Shadows";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Pastel Shadows";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    uint32_t extensionCount = 0;
    static const char *extensions[64] = {0};

    if (!__psAddGlfwExtenstions(&extensionCount, extensions))
    {
        PS_LOG("Failed to add GLFW extensions\n");
        return false;
    }

#ifdef PS_DEBUG
    if (!__psAddDebugUtilsExtenstions(&extensionCount, extensions))
    {
        PS_LOG("Failed to add debug utils extensions\n");
        return false;
    }
#endif

    uint32_t layerCount = 0;
    static const char *layers[64] = {0};

#ifdef PS_DEBUG
    if (!__psAddDebugLayers(&layerCount, layers, &gameState->vulkan.debugLayersEnabled))
    {
        PS_LOG("Failed to add debug layers\n");
        return false;
    }
#endif

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = layers;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, NULL, &gameState->vulkan.instance);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create Vulkan instance\n");
        return false;
    }

#ifdef PS_DEBUG
    if (gameState->vulkan.debugLayersEnabled)
    {
        if (!__psCreateDebugUtilsMessenger(gameState))
        {
            PS_LOG("Failed to create debug utils messenger\n");
            return false;
        }
    }
#endif

    return true;
}

static bool __psVulkanCreateSurface(PS_GameState *gameState)
{
    if (glfwCreateWindowSurface(gameState->vulkan.instance, gameState->window.window, NULL, &gameState->vulkan.surface) != VK_SUCCESS)
    {
        PS_LOG("Failed to create Vulkan surface\n");
        return false;
    }
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

static bool __psVulkanPickPhysicalDevice(PS_GameState *gameState)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(gameState->vulkan.instance, &deviceCount, NULL);
    if (deviceCount == 0)
    {
        PS_LOG("No Vulkan-compatible devices found\n");
        return false;
    }
    if (deviceCount > 64)
    {
        PS_LOG("Too many Vulkan-compatible devices found\n");
        return false;
    }

    VkPhysicalDevice devices[64] = {0};
    vkEnumeratePhysicalDevices(gameState->vulkan.instance, &deviceCount, devices);

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
            gameState->vulkan.physicalDevice = devices[i];
            foundDiscreteGPU = true;
            break;
        }
    }

    if (!foundDiscreteGPU)
    {
        PS_LOG("No discrete GPU found, using first available device\n");
        gameState->vulkan.physicalDevice = devices[0];
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

static bool __psVulkanCreateDevice(PS_GameState *gameState)
{
    int32_t graphicsQueueFamilyIndex = __psVulkanFindQueueFamilyIndex(gameState->vulkan.physicalDevice, VK_QUEUE_GRAPHICS_BIT, gameState->vulkan.surface, -1);
    if (graphicsQueueFamilyIndex < 0)
    {
        PS_LOG("Failed to find graphics queue family index\n");
        return false;
    }

    int32_t computeQueueFamilyIndex = __psVulkanFindQueueFamilyIndex(gameState->vulkan.physicalDevice, VK_QUEUE_COMPUTE_BIT, VK_NULL_HANDLE, graphicsQueueFamilyIndex);
    if (computeQueueFamilyIndex < 0)
    {
        PS_LOG("Failed to find compute queue family index\n");
        return false;
    }

    // We only need one graphics queue and one compute queue for now
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {0};
    float queuePriority = 1.0f;

    //  first the g1raphics queue
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
    if (gameState->vulkan.debugLayersEnabled)
    {
        static const char *debugLayers[] = {
            "VK_LAYER_KHRONOS_validation",
        };
        uint32_t debugLayerCount = PS_ARRAY_COUNT(debugLayers);
        createInfo.ppEnabledLayerNames = debugLayers;
        createInfo.enabledLayerCount = debugLayerCount;
    }    
#endif

    VkResult result = vkCreateDevice(gameState->vulkan.physicalDevice, &createInfo, NULL, &gameState->vulkan.device);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create Vulkan device\n");
        return false;
    }

    gameState->vulkan.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    gameState->vulkan.computeQueueFamilyIndex = computeQueueFamilyIndex;

    return true;
}

static bool __psVulkanGetQueues(PS_GameState *gameState)
{
    vkGetDeviceQueue(gameState->vulkan.device, gameState->vulkan.graphicsQueueFamilyIndex, 0, &gameState->vulkan.graphicsQueue);
    vkGetDeviceQueue(gameState->vulkan.device, gameState->vulkan.computeQueueFamilyIndex, 0, &gameState->vulkan.computeQueue);
    if (gameState->vulkan.graphicsQueue == VK_NULL_HANDLE || gameState->vulkan.computeQueue == VK_NULL_HANDLE)
    {
        PS_LOG("Failed to get Vulkan device queues\n");
        return false;
    }
    return true;
}

static bool __psVulkanCreateCommandPools(PS_GameState *gameState)
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = gameState->vulkan.graphicsQueueFamilyIndex;

    VkResult result = vkCreateCommandPool(gameState->vulkan.device, &poolInfo, NULL, &gameState->vulkan.graphicsCommandPool);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create graphics command pool\n");
        return false;
    }

    poolInfo.queueFamilyIndex = gameState->vulkan.computeQueueFamilyIndex;
    result = vkCreateCommandPool(gameState->vulkan.device, &poolInfo, NULL, &gameState->vulkan.computeCommandPool);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create compute command pool\n");
        return false;
    }

    return true;
}   

bool psVulkanInit(PS_GameState *gameState)
{
    if (!__psVulkanCreateInstance(gameState))
    {
        PS_LOG("Vulkan initialization failed\n");
        return false;
    }

    if (!__psVulkanCreateSurface(gameState))
    {
        PS_LOG("Failed to create Vulkan surface\n");
        return false;
    }

    if (!__psVulkanPickPhysicalDevice(gameState))
    {
        PS_LOG("Failed to pick physical device\n");
        return false;
    }

    if (!__psVulkanCreateDevice(gameState))
    {
        PS_LOG("Failed to create Vulkan device\n");
        return false;
    }

    if (!__psVulkanGetQueues(gameState))
    {
        PS_LOG("Failed to get Vulkan device queues\n");
        return false;
    }

    if (!__psVulkanCreateCommandPools(gameState))
    {
        PS_LOG("Failed to create Vulkan command pools\n");
        return false;
    }

    return true;
}

void psVulkanShutdown(PS_GameState *gameState)
{
    vkDeviceWaitIdle(gameState->vulkan.device);

    vkDestroyCommandPool(gameState->vulkan.device, gameState->vulkan.graphicsCommandPool, NULL);
    gameState->vulkan.graphicsCommandPool = VK_NULL_HANDLE;

    vkDestroyCommandPool(gameState->vulkan.device, gameState->vulkan.computeCommandPool, NULL);
    gameState->vulkan.computeCommandPool = VK_NULL_HANDLE;

    vkDestroyDevice(gameState->vulkan.device, NULL);
    gameState->vulkan.device = VK_NULL_HANDLE;

#ifdef PS_DEBUG
    if (gameState->vulkan.debugLayersEnabled)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(gameState->vulkan.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebugUtilsMessenger != NULL)
        {
            destroyDebugUtilsMessenger(gameState->vulkan.instance, gameState->vulkan.debugMessenger, NULL);
            gameState->vulkan.debugMessenger = VK_NULL_HANDLE;
        }
    }
#endif

    vkDestroySurfaceKHR(gameState->vulkan.instance, gameState->vulkan.surface, NULL);
    gameState->vulkan.surface = VK_NULL_HANDLE;

    vkDestroyInstance(gameState->vulkan.instance, NULL);
    gameState->vulkan.instance = VK_NULL_HANDLE;
}