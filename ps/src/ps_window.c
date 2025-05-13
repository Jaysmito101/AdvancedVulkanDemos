#include "ps_window.h"
#include "ps_application.h"

#include <glfw/glfw3.h>


static void __psGLFWErrorCallback(int error, const char *description)
{
    PS_LOG("GLFW Error: %d - %s\n", error, description);
}

static void __psGLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    if (key >= 0 && key < 1024) {
        gameState->input.keyState[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
}

static void __psGLFWCharCallback(GLFWwindow *window, unsigned int codepoint)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle char events
    // PS_LOG("Char: %u\n", codepoint);
}

static void __psGLFWDropCallback(GLFWwindow *window, int count, const char **paths)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle drop events
    // PS_LOG("Drop: %d files\n", count);
    // for (int i = 0; i < count; ++i) {
    //     PS_LOG("  %s\n", paths[i]);
    // }
}

static void __psGLFWScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    gameState->input.mouseScrollX = (float)xoffset;
    gameState->input.mouseScrollY = (float)yoffset;
}

static void __psGLFWCursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    psInputCalculateMousePositionFromRaw(gameState, xpos, ypos);
}

static void __psGLFWWindowPosCallback(GLFWwindow *window, int xpos, int ypos)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window position events
    // PS_LOG("Window Pos: x=%d, y=%d\n", xpos, ypos);
}

static void __psGLFWWindowSizeCallback(GLFWwindow *window, int width, int height)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    gameState->window.width = width;
    gameState->window.height = height;
    gameState->vulkan.swapchain.swapchainRecreateRequired = true;
    if (width == 0 || height == 0) {
        gameState->window.isMinimized = true;
    } else {
        gameState->window.isMinimized = false;
        psInputCalculateMousePositionFromRaw(gameState, gameState->input.rawMouseX, gameState->input.rawMouseY);
    }
}

static void __psGLFWCursorEnterCallback(GLFWwindow *window, int entered)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle cursor enter/leave events
    // PS_LOG("Cursor Enter: %d\n", entered);
}

static void __psGLFWWindowCloseCallback(GLFWwindow *window)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    gameState->running = false; // Signal the main loop to exit
}

static void __psGLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    gameState->input.mouseButtonState[button] = (action == GLFW_PRESS);
}

static void __psGLFWFramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
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

static void __psGLFWWindowMaximizeCallback(GLFWwindow *window, int maximized)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window maximize/restore events
    // PS_LOG("Window Maximize: %d\n", maximized);
}

static void __psGLFWWindowFocusCallback(GLFWwindow *window, int focused)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle window focus/blur events
    // PS_LOG("Window Focus: %d\n", focused);
}

// ---------- GLFW Callbacks End ----------


static void __psSetupWindowEvents(PS_Window *window) {
    PS_ASSERT(window != NULL);

    glfwSetErrorCallback(__psGLFWErrorCallback);
    glfwSetKeyCallback(window->window, __psGLFWKeyCallback);
    glfwSetCharCallback(window->window, __psGLFWCharCallback);
    glfwSetDropCallback(window->window, __psGLFWDropCallback);
    glfwSetScrollCallback(window->window, __psGLFWScrollCallback);
    glfwSetCursorPosCallback(window->window, __psGLFWCursorPosCallback);
    glfwSetWindowPosCallback(window->window, __psGLFWWindowPosCallback);
    glfwSetWindowSizeCallback(window->window, __psGLFWWindowSizeCallback);
    glfwSetCursorEnterCallback(window->window, __psGLFWCursorEnterCallback);
    glfwSetWindowCloseCallback(window->window, __psGLFWWindowCloseCallback);
    glfwSetMouseButtonCallback(window->window, __psGLFWMouseButtonCallback);
    glfwSetFramebufferSizeCallback(window->window, __psGLFWFramebufferSizeCallback);
    glfwSetWindowMaximizeCallback(window->window, __psGLFWWindowMaximizeCallback);
    glfwSetWindowFocusCallback(window->window, __psGLFWWindowFocusCallback);
}

bool psWindowInit(PS_Window* window, PS_GameState *gameState) {

    if(!glfwInit()) {
        PS_LOG("Failed to initialize GLFW\n");
        return false;
    }

    if(!glfwVulkanSupported()) {
        PS_LOG("Vulkan is not supported\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window->window = glfwCreateWindow(1280, 720, "Pastel Shadows", NULL, NULL);
    if (!window->window) {
        PS_LOG("Failed to create GLFW window\n");
        glfwTerminate(); // Terminate GLFW if window creation fails
        return false;
    }
    window->width = 1280; // Store initial size
    window->height = 720;
    window->isMinimized = false;
    glfwSetWindowUserPointer(window->window, gameState);

    __psSetupWindowEvents(window);

    return true;
}

void psWindowShutdown(PS_Window *window) {
    PS_ASSERT(window != NULL);

    if (window->window) {
        glfwDestroyWindow(window->window);
        window->window = NULL;
    }
    glfwTerminate();
}

void psWindowPollEvents() {
    glfwPollEvents();
}