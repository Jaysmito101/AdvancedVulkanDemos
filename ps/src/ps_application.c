#include "ps_application.h"
#include "ps_window.h"
#include "ps_vulkan.h"

#include "glfw/glfw3.h"

static void __psApplicationUpdateFramerateCalculation(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    double currentTime = glfwGetTime();
    gameState->framerate.currentTime = currentTime;
    gameState->framerate.deltaTime = currentTime - gameState->framerate.lastTime;
    gameState->framerate.lastTime = currentTime;
    gameState->framerate.lastSecondFrameCounter++;
    gameState->framerate.instanteneousFrameRate = (size_t)(1.0 / gameState->framerate.deltaTime);

    if (currentTime - gameState->framerate.lastSecondTime >= 1.0) {
        gameState->framerate.fps = gameState->framerate.lastSecondFrameCounter;
        gameState->framerate.lastSecondFrameCounter = 0;
        gameState->framerate.lastSecondTime = currentTime;

        // update the title of window with stats
        static char title[256];
        snprintf(title, sizeof(title), "Pastel Shadows -- FPS(Stable): %zu, FPS(Instant): %zu, DeltaTime: %.3f", gameState->framerate.fps, gameState->framerate.instanteneousFrameRate, gameState->framerate.deltaTime);
        glfwSetWindowTitle(gameState->window.window, title);
    }
    
}

bool psApplicationInit(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    gameState->running = true;

    if(!psWindowInit(gameState)) {
        PS_LOG("Failed to initialize window\n");
        return false;
    }

    if(!psVulkanInit(gameState)) {
        PS_LOG("Failed to initialize Vulkan\n");
        return false;
    }

    // Initialize other application components here

    return true;
}

void psApplicationShutdown(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    psVulkanShutdown(gameState);    
    psWindowShutdown(gameState);
}

bool psApplicationIsRunning(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    return gameState->running;
}

void psApplicationUpdate(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    __psApplicationUpdateFramerateCalculation(gameState);

    psWindowPollEvents(gameState);
    psApplicationUpdateWithoutPolling(gameState);
}



void psApplicationUpdateWithoutPolling(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    psApplicationRender(gameState);
}


void psApplicationRender(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    // to not render if the window is minimized
    if (gameState->window.isMinimized) {
        return;
    }    
       
    psVulkanRendererRender(gameState);
}