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
} PS_Window;

typedef struct PS_Vulkan {
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;

#ifdef PS_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
} PS_Vulkan;

typedef struct PS_GameState {
    PS_Window window;
    PS_Vulkan vulkan;

    bool running;
} PS_GameState;


#endif // PS_GAME_STATE_H