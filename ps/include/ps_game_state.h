#ifndef PS_GAME_STATE_H
#define PS_GAME_STATE_H

#define API_VERSION VK_API_VERSION_1_2
#include <vulkan/vulkan.h>

struct GLFWwindow;

typedef struct PS_Window {
    struct GLFWwindow* window;
    int32_t width;
    int32_t height;

    int32_t framebufferWidth;
    int32_t framebufferHeight;

    bool isMinimized;
} PS_Window;

typedef struct PS_VulkanSwapchain {
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage images[64];
    VkImageView imageViews[64];
    VkFramebuffer framebuffers[64];
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    uint32_t imageCount;
    VkExtent2D extent;
    VkRenderPass renderPass;
    bool swapchainRecreateRequired;
    bool swapchainReady;
} PS_VulkanSwapchain;

typedef struct PS_VulkanRenderer {
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence renderFence;
} PS_VulkanRenderer;

typedef struct PS_Vulkan {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;

    
    PS_VulkanSwapchain swapchain;

    int32_t graphicsQueueFamilyIndex;
    int32_t computeQueueFamilyIndex;

#ifdef PS_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    bool debugLayersEnabled;
#endif
} PS_Vulkan;


typedef struct PS_Frametime {
    double lastTime;
    double currentTime;
    double deltaTime;
    
    size_t instanteneousFrameRate;
    size_t fps;
    size_t lastSecondFrameCounter;
    double lastSecondTime;
} PS_Frametime;

typedef struct PS_GameState {
    PS_Window window;
    PS_Vulkan vulkan;
    PS_Frametime framerate;

    bool running;
} PS_GameState;


#endif // PS_GAME_STATE_H