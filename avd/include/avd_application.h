#ifndef AVD_APPLICATION_H
#define AVD_APPLICATION_H

#include "core/avd_core.h"
#include "font/avd_font_renderer.h"
#include "scenes/avd_scenes.h"
#include "shader/avd_shader.h"
#include "ui/avd_ui.h"
#include "vulkan/avd_vulkan.h"

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
    AVD_Window window;                   // The window handle
    AVD_Input input;                     // Input manager
    AVD_Frametime framerate;             // The general statistics tracker
    AVD_Vulkan vulkan;                   // The Vulkan device and context
    AVD_VulkanSwapchain swapchain;       // The Vulkan swapchain
    AVD_VulkanRenderer renderer;         // The Vulkan renderer
    AVD_VulkanPresentation presentation; // The Vulkan presentation (system used to show rendered images on screen)
    AVD_FontManager fontManager;         // The font manager
    AVD_FontRenderer fontRenderer;       // The default font renderer used for rendering text
    AVD_SceneManager sceneManager;       // The scene manager
    AVD_ShaderManager shaderManager;     // The shader manager
    AVD_Ui ui;                           // The UI manager

    VkSurfaceKHR surface; // The Vulkan surface
    bool running;         // The application running state
} AVD_AppState;

bool avdApplicationInit(AVD_AppState *appState);
void avdApplicationShutdown(AVD_AppState *appState);

bool avdApplicationIsRunning(AVD_AppState *appState);
void avdApplicationUpdate(AVD_AppState *appState);
void avdApplicationUpdateWithoutPolling(AVD_AppState *appState);
void avdApplicationRender(AVD_AppState *appState);

#endif // AVD_APPLICATION_H