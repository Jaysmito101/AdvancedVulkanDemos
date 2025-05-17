#include "core/avd_window.h"
#include "core/avd_window_cb.h"

bool avdWindowInit(AVD_Window *window, AVD_AppState *appState)
{
    if (!glfwInit()) {
        AVD_LOG("Failed to initialize GLFW\n");
        return false;
    }

    if (!glfwVulkanSupported()) {
        AVD_LOG("Vulkan is not supported\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window->window = glfwCreateWindow(1280, 720, "Pastel Shadows", NULL, NULL);
    if (!window->window) {
        AVD_LOG("Failed to create GLFW window\n");
        glfwTerminate(); // Terminate GLFW if window creation fails
        return false;
    }
    window->width       = 1280; // Store initial size
    window->height      = 720;
    window->isMinimized = false;
    glfwSetWindowUserPointer(window->window, appState);

    __avdSetupWindowEvents(window);

    return true;
}

void avdWindowShutdown(AVD_Window *window)
{
    AVD_ASSERT(window != NULL);

    if (window->window) {
        glfwDestroyWindow(window->window);
        window->window = NULL;
    }
    glfwTerminate();
}

void avdWindowPollEvents()
{
    glfwPollEvents();
}