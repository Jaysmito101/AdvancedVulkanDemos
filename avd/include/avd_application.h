#ifndef AVD_APPLICATION_H
#define AVD_APPLICATION_H


#include "core/avd_core.h"
#include "vulkan/avd_vulkan.h"
#include "font/avd_font_renderer.h"
#include "bloom/avd_bloom.h"


typedef struct AVD_Frametime {
    double lastTime;
    double currentTime;
    double deltaTime;
    
    size_t instanteneousFrameRate;
    size_t fps;
    size_t lastSecondFrameCounter;
    double lastSecondTime;
} AVD_Frametime;

typedef struct AVD_AppState {
    AVD_Window window;                      // The window handle
    AVD_Input input;                        // Input manager
    AVD_Frametime framerate;                // The general statistics tracker
    AVD_Vulkan vulkan;                      // The Vulkan device and context
    AVD_VulkanSwapchain swapchain;          // The Vulkan swapchain
    AVD_VulkanRenderer renderer;            // The Vulkan renderer
    AVD_VulkanPresentation presentation;    // The Vulkan presentation (system used to show rendered images on screen)
    AVD_FontRenderer fontRenderer;          // The font renderer
    AVD_Bloom bloom;                        // The bloom effect

    VkSurfaceKHR surface;                   // The Vulkan surface
    bool running;                           // The application running state
} AVD_AppState;

bool avdApplicationInit(AVD_AppState *appState);
void avdApplicationShutdown(AVD_AppState *appState);

bool avdApplicationIsRunning(AVD_AppState *appState);
void avdApplicationUpdate(AVD_AppState *appState);
void avdApplicationUpdateWithoutPolling(AVD_AppState *appState);
void avdApplicationRender(AVD_AppState *appState);

#endif // AVD_APPLICATION_H