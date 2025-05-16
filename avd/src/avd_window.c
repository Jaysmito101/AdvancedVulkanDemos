#include "avd_window.h"
#include "avd_application.h"

#include <glfw/glfw3.h>


static void __avdGLFWErrorCallback(int error, const char *description)
{
    AVD_LOG("GLFW Error: %d - %s\n", error, description);
}

static void __avdGLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    if (key >= 0 && key < 1024) {
        gameState->input.keyState[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
}

static void __avdGLFWCharCallback(GLFWwindow *window, unsigned int codepoint)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle char events
    // AVD_LOG("Char: %u\n", codepoint);
}

static void __avdGLFWDropCallback(GLFWwindow *window, int count, const char **paths)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle drop events
    // AVD_LOG("Drop: %d files\n", count);
    // for (int i = 0; i < count; ++i) {
    //     AVD_LOG("  %s\n", paths[i]);
    // }
}

static void __avdGLFWScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    gameState->input.mouseScrollX = (float)xoffset;
    gameState->input.mouseScrollY = (float)yoffset;
}

static void __avdGLFWCursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    avdInputCalculateMousePositionFromRaw(gameState, xpos, ypos);
}

static void __avdGLFWWindowPosCallback(GLFWwindow *window, int xpos, int ypos)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window position events
    // AVD_LOG("Window Pos: x=%d, y=%d\n", xpos, ypos);
}

static void __avdGLFWWindowSizeCallback(GLFWwindow *window, int width, int height)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    gameState->window.width = width;
    gameState->window.height = height;
    gameState->vulkan.swapchain.swapchainRecreateRequired = true;
    if (width == 0 || height == 0) {
        gameState->window.isMinimized = true;
    } else {
        gameState->window.isMinimized = false;
        avdInputCalculateMousePositionFromRaw(gameState, gameState->input.rawMouseX, gameState->input.rawMouseY);
    }
}

static void __avdGLFWCursorEnterCallback(GLFWwindow *window, int entered)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle cursor enter/leave events
    // AVD_LOG("Cursor Enter: %d\n", entered);
}

static void __avdGLFWWindowCloseCallback(GLFWwindow *window)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    gameState->running = false; // Signal the main loop to exit
}

static void __avdGLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    gameState->input.mouseButtonState[button] = (action == GLFW_PRESS);
}

static void __avdGLFWFramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle framebuffer size changes (important for rendering viewport)
    gameState->window.framebufferWidth = width;
    gameState->window.framebufferHeight = height;
    gameState->vulkan.swapchain.swapchainRecreateRequired = true;
    if (width == 0 || height == 0) {
        gameState->window.isMinimized = true;
    } else {
        gameState->window.isMinimized = false;
    }
}

static void __avdGLFWWindowMaximizeCallback(GLFWwindow *window, int maximized)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window maximize/restore events
    // AVD_LOG("Window Maximize: %d\n", maximized);
}

static void __avdGLFWWindowFocusCallback(GLFWwindow *window, int focused)
{
    AVD_GameState *gameState = (AVD_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window focus/blur events
    // AVD_LOG("Window Focus: %d\n", focused);
}

// ---------- GLFW Callbacks End ----------


static void __avdSetupWindowEvents(AVD_Window *window) {
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

bool avdWindowInit(AVD_Window* window, AVD_GameState *gameState) {

    if(!glfwInit()) {
        AVD_LOG("Failed to initialize GLFW\n");
        return false;
    }

    if(!glfwVulkanSupported()) {
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
    window->width = 1280; // Store initial size
    window->height = 720;
    window->isMinimized = false;
    glfwSetWindowUserPointer(window->window, gameState);

    __avdSetupWindowEvents(window);

    return true;
}

void avdWindowShutdown(AVD_Window *window) {
    AVD_ASSERT(window != NULL);

    if (window->window) {
        glfwDestroyWindow(window->window);
        window->window = NULL;
    }
    glfwTerminate();
}

void avdWindowPollEvents() {
    glfwPollEvents();
}