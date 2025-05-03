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
    // TODO: Handle key events
    // PS_LOG("Key: %d, Scancode: %d, Action: %d, Mods: %d\n", key, scancode, action, mods);
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
    // TODO: Handle scroll events
    // PS_LOG("Scroll: x=%.2f, y=%.2f\n", xoffset, yoffset);
}

static void __psGLFWCursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle cursor position events
    // PS_LOG("Cursor Pos: x=%.2f, y=%.2f\n", xpos, ypos);
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
    PS_LOG("Window close requested.\n");
    gameState->running = false; // Signal the main loop to exit
}

static void __psGLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle mouse button events
    // PS_LOG("Mouse Button: %d, Action: %d, Mods: %d\n", button, action, mods);
}

static void __psGLFWFramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    PS_GameState *gameState = (PS_GameState *)glfwGetWindowUserPointer(window);
    // TODO: Handle framebuffer size changes (important for rendering viewport)
    gameState->window.framebufferWidth = width;
    gameState->window.framebufferHeight = height;
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


static void __psSetupWindowEvents(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(gameState->window.window != NULL);

    glfwSetErrorCallback(__psGLFWErrorCallback);
    glfwSetKeyCallback(gameState->window.window, __psGLFWKeyCallback);
    glfwSetCharCallback(gameState->window.window, __psGLFWCharCallback);
    glfwSetDropCallback(gameState->window.window, __psGLFWDropCallback);
    glfwSetScrollCallback(gameState->window.window, __psGLFWScrollCallback);
    glfwSetCursorPosCallback(gameState->window.window, __psGLFWCursorPosCallback);
    glfwSetWindowPosCallback(gameState->window.window, __psGLFWWindowPosCallback);
    glfwSetWindowSizeCallback(gameState->window.window, __psGLFWWindowSizeCallback);
    glfwSetCursorEnterCallback(gameState->window.window, __psGLFWCursorEnterCallback);
    glfwSetWindowCloseCallback(gameState->window.window, __psGLFWWindowCloseCallback);
    glfwSetMouseButtonCallback(gameState->window.window, __psGLFWMouseButtonCallback);
    glfwSetFramebufferSizeCallback(gameState->window.window, __psGLFWFramebufferSizeCallback);
    glfwSetWindowMaximizeCallback(gameState->window.window, __psGLFWWindowMaximizeCallback);
    glfwSetWindowFocusCallback(gameState->window.window, __psGLFWWindowFocusCallback);
}

bool psWindowInit(PS_GameState *gameState) {

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

    gameState->window.window = glfwCreateWindow(1280, 720, "Pixel Space", NULL, NULL);
    if (!gameState->window.window) {
        PS_LOG("Failed to create GLFW window\n");
        glfwTerminate(); // Terminate GLFW if window creation fails
        return false;
    }
    gameState->window.width = 1280; // Store initial size
    gameState->window.height = 720;
    glfwSetWindowUserPointer(gameState->window.window, gameState);

    __psSetupWindowEvents(gameState); // Setup callbacks

    return true;
}

void psWindowShutdown(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    if (gameState->window.window) {
        glfwDestroyWindow(gameState->window.window);
        gameState->window.window = NULL;
    }
    glfwTerminate();
}

void psWindowPollEvents(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    glfwPollEvents();
}