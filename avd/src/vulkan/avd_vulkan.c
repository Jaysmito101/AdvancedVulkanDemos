#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"

static const char *__avd_RequiredVulkanExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, // Disabled for RenderDoc support!
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
    VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME};

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
    AVD_LOG_DEBUG("%s: %s", severity, pCallbackData->pMessage);
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

static bool __avdVulkanPhysicalDeviceCheckExtensions(VkPhysicalDevice device)
{
    uint32_t extensionCount                      = 0;
    static VkExtensionProperties extensions[256] = {0};
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
    if (extensionCount == 0) {
        AVD_LOG_ERROR("No Vulkan-compatible extensions found");
        return false;
    }
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, extensions);

    for (uint32_t i = 0; i < AVD_ARRAY_COUNT(__avd_RequiredVulkanExtensions); ++i) {
        bool found = false;
        for (uint32_t j = 0; j < extensionCount; ++j) {
            if (strcmp(__avd_RequiredVulkanExtensions[i], extensions[j].extensionName) == 0) {
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

static bool __avdVulkanPhysicalDeviceCheckFeatures(VkPhysicalDevice device)
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

    bool foundDiscreteGPU = false;
    for (uint32_t i = 0; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        if (!__avdVulkanPhysicalDeviceCheckExtensions(devices[i])) {
            AVD_LOG_WARN("Physical device %s does not support required extensions", deviceProperties.deviceName);
            continue;
        }

        if (!__avdVulkanPhysicalDeviceCheckFeatures(devices[i])) {
            AVD_LOG_WARN("Physical device %s does not support required features", deviceProperties.deviceName);
            continue;
        }

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            vulkan->physicalDevice = devices[i];
            foundDiscreteGPU       = true;
            break;
        }
    }

    if (!foundDiscreteGPU) {
        AVD_LOG_WARN("No discrete GPU found, using first available device");
        vulkan->physicalDevice = devices[0];
    }

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

    // We only need one graphics queue and one compute queue for now
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {0};
    float queuePriority                         = 1.0f;

    //  first the graphics queue
    queueCreateInfos[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfos[0].queueCount       = 1;
    queueCreateInfos[0].pQueuePriorities = &queuePriority;

    // then the compute queue
    queueCreateInfos[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[1].queueFamilyIndex = computeQueueFamilyIndex;
    queueCreateInfos[1].queueCount       = 1;
    queueCreateInfos[1].pQueuePriorities = &queuePriority;

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

    VkPhysicalDeviceFeatures2 deviceFeatures2        = {0};
    deviceFeatures2.sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features.multiDrawIndirect       = VK_TRUE;
    deviceFeatures2.features.pipelineStatisticsQuery = VK_TRUE;
    deviceFeatures2.features.samplerAnisotropy       = VK_TRUE;
    deviceFeatures2.features.shaderInt64             = VK_TRUE;
    deviceFeatures2.features.shaderInt16             = VK_TRUE;
    deviceFeatures2.pNext                            = &deviceVulkan11Features;

    VkDeviceCreateInfo createInfo      = {0};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = 2;
    createInfo.pQueueCreateInfos       = queueCreateInfos;
    createInfo.enabledExtensionCount   = AVD_ARRAY_COUNT(__avd_RequiredVulkanExtensions);
    createInfo.ppEnabledExtensionNames = __avd_RequiredVulkanExtensions;
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

    vulkan->graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    vulkan->computeQueueFamilyIndex  = computeQueueFamilyIndex;

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
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex        = vulkan->graphicsQueueFamilyIndex;

    VkResult result = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->graphicsCommandPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics command pool\n");

    poolInfo.queueFamilyIndex = vulkan->computeQueueFamilyIndex;
    result                    = vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->computeCommandPool);
    AVD_CHECK_VK_RESULT(result, "Failed to create compute command pool\n");

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