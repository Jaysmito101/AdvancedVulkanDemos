#include "ps_window.h"

#include <glfw/glfw3.h>


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
        return false;
    }
    glfwSetWindowUserPointer(gameState->window.window, gameState);
    glfwMakeContextCurrent(gameState->window.window);
    glfwSwapInterval(1);

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

    // Check if the window should close
    if (glfwWindowShouldClose(gameState->window.window)) {
        gameState->running = false;
    }
}