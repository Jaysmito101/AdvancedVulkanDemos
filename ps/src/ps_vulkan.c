#include "ps_vulkan.h"
#include "glfw/glfw3.h"


static bool __psVulkanLayersSupported(const char **layers, uint32_t layerCount) {
    static VkLayerProperties availableLayers[128] = {0};
    uint32_t availableLayerCount = 0;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
    if (availableLayerCount > PS_ARRAY_COUNT(availableLayers)) {
        PS_LOG("Too many available layers\n");
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
            PS_LOG("Layer %s not supported\n", layers[i]);
            return false;
        }
    }    
    return true;
}

static bool __psAddGlfwExtenstions(uint32_t *extensionCount, const char **extensions) {
    uint32_t count = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    if (glfwExtensions == NULL) {
        PS_LOG("Failed to get GLFW extensions\n");
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        extensions[i + *extensionCount] = glfwExtensions[i];
    }
    *extensionCount += count;
    return true;
}

#ifdef PS_DEBUG
static bool __psAddDebugUtilsExtenstions(uint32_t *extensionCount, const char **extensions) {
    extensions[*extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    (*extensionCount)++;
    return true;
}

static bool __psAddDebugLayers(uint32_t *layerCount, const char **layers) {
    static const char *debugLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    if (__psVulkanLayersSupported(debugLayers, PS_ARRAY_COUNT(debugLayers))) {
        for (uint32_t i = 0; i < PS_ARRAY_COUNT(debugLayers); ++i) {
            layers[i + *layerCount] = debugLayers[i];
        }
        *layerCount += PS_ARRAY_COUNT(debugLayers);
    } else {
        PS_LOG("Debug layers not supported\n");
    }
    return true;
}

static VkBool32 __psDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                                           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                           void *pUserData) {
    const char *severity = NULL;
    switch (messageSeverity) {
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

static bool __psCreateDebugUtilsMessenger(PS_GameState *gameState) {
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
    if (createDebugUtilsMessenger == NULL) {
        PS_LOG("Failed to load vkCreateDebugUtilsMessengerEXT\n");
        return false;
    }
    VkResult result = createDebugUtilsMessenger(gameState->vulkan.instance, &createInfo, NULL, &gameState->vulkan.debugMessenger);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create debug utils messenger\n");
        return false;
    }
    return true;
}

#endif



static bool __psVulkanCreateInstance(PS_GameState* gameState) {
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Pastel Shadows";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Pastel Shadows";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;
    
    uint32_t extensionCount = 0;
    static const char *extensions[64] = {0};

    if(!__psAddGlfwExtenstions(&extensionCount, extensions)) {
        PS_LOG("Failed to add GLFW extensions\n");
        return false;
    }

#ifdef PS_DEBUG
    if(!__psAddDebugUtilsExtenstions(&extensionCount, extensions)) {
        PS_LOG("Failed to add debug utils extensions\n");
        return false;
    }
#endif

    uint32_t layerCount = 0;
    static const char *layers[64] = { 0 };

#ifdef PS_DEBUG
    if (!__psAddDebugLayers(&layerCount, layers)) {
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
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create Vulkan instance\n");
        return false;
    }

#ifdef PS_DEBUG
    if (!__psCreateDebugUtilsMessenger(gameState)) {
        PS_LOG("Failed to create debug utils messenger\n");
        return false;
    }
#endif

    return true;
}

bool psVulkanInit(PS_GameState *gameState) {
    if (!__psVulkanCreateInstance(gameState)) {
        PS_LOG("Vulkan initialization failed\n");
        return false;
    }

    return true;
}

void psVulkanShutdown(PS_GameState *gameState) {
    //vkDeviceWaitIdle(gameState->vulkan.device);

    
    //vkDestroyDevice(gameState->vulkan.device, NULL);
    gameState->vulkan.device = VK_NULL_HANDLE;
    
#ifdef PS_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger = 
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(gameState->vulkan.instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroyDebugUtilsMessenger != NULL) {
        destroyDebugUtilsMessenger(gameState->vulkan.instance, gameState->vulkan.debugMessenger, NULL);
        gameState->vulkan.debugMessenger = VK_NULL_HANDLE;
    }
#endif

    vkDestroyInstance(gameState->vulkan.instance, NULL);
    gameState->vulkan.instance = VK_NULL_HANDLE;
}