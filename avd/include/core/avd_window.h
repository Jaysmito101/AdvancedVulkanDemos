#ifndef AVD_WINDOW_H
#define AVD_WINDOW_H

#include "core/avd_base.h"

#include <GLFW/glfw3.h>

struct AVD_AppState;

typedef struct AVD_Window {
    GLFWwindow *window;
    int32_t width;
    int32_t height;

    int32_t framebufferWidth;
    int32_t framebufferHeight;

    bool isMinimized;
} AVD_Window;

bool avdWindowInit(AVD_Window *window, struct AVD_AppState *gameState);
void avdWindowShutdown(AVD_Window *window);
void avdWindowPollEvents();

#endif // AVD_WINDOW_H
