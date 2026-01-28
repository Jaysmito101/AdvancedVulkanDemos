#include "core/avd_window_cb.h"
#include "avd_application.h"
#include "core/avd_window.h"

void __avdGLFWErrorCallback(int error, const char *description)
{
    AVD_LOG_ERROR("GLFW Error: %d - %s", error, description);
}

void __avdGLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    if (key >= 0 && key < 1024) {
        appState->input.keyState[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    AVD_InputEvent event;
    event.type         = AVD_INPUT_EVENT_KEY;
    event.key.key      = key;
    event.key.scancode = scancode;
    event.key.action   = action;
    event.key.mods     = mods;
    avdSceneManagerPushInputEvent(&appState->sceneManager, appState, &event);
}

void __avdGLFWCharCallback(GLFWwindow *window, unsigned int codepoint)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    // TODO: Handle char events
    // AVD_LOG("Char: %u\n", codepoint);
}

void __avdGLFWDropCallback(GLFWwindow *window, int count, const char **paths)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    // TODO: Handle drop events
    // AVD_LOG("Drop: %d files\n", count);
    // for (int i = 0; i < count; ++i) {
    //     AVD_LOG("  %s\n", paths[i]);
    // }
    AVD_InputEvent event;
    event.type            = AVD_INPUT_EVENT_DRAG_N_DROP;
    event.dragNDrop.count = count;
    event.dragNDrop.paths = paths;
    avdSceneManagerPushInputEvent(&appState->sceneManager, appState, &event);
}

void __avdGLFWScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    AVD_AppState *appState       = (AVD_AppState *)glfwGetWindowUserPointer(window);
    appState->input.mouseScrollX = (float)xoffset;
    appState->input.mouseScrollY = (float)yoffset;

    AVD_InputEvent event;
    event.type          = AVD_INPUT_EVENT_MOUSE_SCROLL;
    event.mouseScroll.x = (float)xoffset;
    event.mouseScroll.y = (float)yoffset;
    avdSceneManagerPushInputEvent(&appState->sceneManager, appState, &event);
}

void __avdGLFWCursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    avdInputCalculateMousePositionFromRaw(&appState->input, &appState->window, xpos, ypos);

    AVD_InputEvent event;
    event.type        = AVD_INPUT_EVENT_MOUSE_MOVE;
    event.mouseMove.x = appState->input.mouseX;
    event.mouseMove.y = appState->input.mouseY;
    avdSceneManagerPushInputEvent(&appState->sceneManager, appState, &event);
}

void __avdGLFWWindowPosCallback(GLFWwindow *window, int xpos, int ypos)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window position events
    // AVD_LOG("Window Pos: x=%d, y=%d\n", xpos, ypos);
    AVD_InputEvent event;
    event.type         = AVD_INPUT_EVENT_WINDOW_MOVE;
    event.windowMove.x = xpos;
    event.windowMove.y = ypos;
    avdSceneManagerPushInputEvent(&appState->sceneManager, appState, &event);
}

void __avdGLFWWindowSizeCallback(GLFWwindow *window, int width, int height)
{
    AVD_AppState *appState  = (AVD_AppState *)glfwGetWindowUserPointer(window);
    appState->window.width  = width;
    appState->window.height = height;

    // appState->vulkan.swapchain.swapchainRecreateRequired = true;

    if (width == 0 || height == 0) {
        appState->window.isMinimized = true;
    } else {
        appState->window.isMinimized = false;
        avdInputCalculateMousePositionFromRaw(&appState->input, &appState->window, appState->input.rawMouseX, appState->input.rawMouseY);
    }

    AVD_InputEvent event;
    event.type                = AVD_INPUT_EVENT_WINDOW_RESIZE;
    event.windowResize.width  = width;
    event.windowResize.height = height;

    avdSceneManagerPushInputEvent(&appState->sceneManager, appState, &event);

    appState->swapchain.swapchainRecreateRequired = true;
    avdInputCalculateDeltas(&appState->input);
    avdApplicationUpdateWithoutPolling(appState);
}

void __avdGLFWCursorEnterCallback(GLFWwindow *window, int entered)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    // TODO: Handle cursor enter/leave events
    // AVD_LOG("Cursor Enter: %d\n", entered);
}

void __avdGLFWWindowCloseCallback(GLFWwindow *window)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    appState->running      = false; // Signal the main loop to exit

    AVD_InputEvent event;
    event.type = AVD_INPUT_EVENT_WINDOW_CLOSE;
    avdSceneManagerPushInputEvent(&appState->sceneManager, appState, &event);
}

void __avdGLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    AVD_AppState *appState                   = (AVD_AppState *)glfwGetWindowUserPointer(window);
    appState->input.mouseButtonState[button] = (action == GLFW_PRESS);

    AVD_InputEvent event;
    event.type               = AVD_INPUT_EVENT_MOUSE_BUTTON;
    event.mouseButton.button = button;
    event.mouseButton.action = action;
    event.mouseButton.mods   = mods;
    avdSceneManagerPushInputEvent(&appState->sceneManager, appState, &event);
}

void __avdGLFWFramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    // TODO: Handle framebuffer size changes (important for rendering viewport)
    appState->window.framebufferWidth  = width;
    appState->window.framebufferHeight = height;

    // appState->vulkan.swapchain.swapchainRecreateRequired = true;

    if (width == 0 || height == 0) {
        appState->window.isMinimized = true;
    } else {
        appState->window.isMinimized = false;
    }
}

void __avdGLFWWindowMaximizeCallback(GLFWwindow *window, int maximized)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window maximize/restore events
    // AVD_LOG("Window Maximize: %d\n", maximized);
}

void __avdGLFWWindowFocusCallback(GLFWwindow *window, int focused)
{
    AVD_AppState *appState = (AVD_AppState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window focus/blur events
    // AVD_LOG("Window Focus: %d\n", focused);
}

// ---------- GLFW Callbacks End ----------

void __avdSetupWindowEvents(AVD_Window *window)
{
    AVD_ASSERT(window != NULL);

    glfwSetErrorCallback(__avdGLFWErrorCallback);
    glfwSetKeyCallback(window->window, __avdGLFWKeyCallback);
    glfwSetCharCallback(window->window, __avdGLFWCharCallback);
    glfwSetDropCallback(window->window, __avdGLFWDropCallback);
    glfwSetScrollCallback(window->window, __avdGLFWScrollCallback);
    glfwSetCursorPosCallback(window->window, __avdGLFWCursorPosCallback);
    glfwSetWindowPosCallback(window->window, __avdGLFWWindowPosCallback);
    glfwSetWindowSizeCallback(window->window, __avdGLFWWindowSizeCallback);
    glfwSetCursorEnterCallback(window->window, __avdGLFWCursorEnterCallback);
    glfwSetWindowCloseCallback(window->window, __avdGLFWWindowCloseCallback);
    glfwSetMouseButtonCallback(window->window, __avdGLFWMouseButtonCallback);
    glfwSetFramebufferSizeCallback(window->window, __avdGLFWFramebufferSizeCallback);
    glfwSetWindowMaximizeCallback(window->window, __avdGLFWWindowMaximizeCallback);
    glfwSetWindowFocusCallback(window->window, __avdGLFWWindowFocusCallback);
}
