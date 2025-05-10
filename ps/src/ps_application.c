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

    PS_CHECK(psWindowInit(gameState));
    PS_CHECK(psVulkanInit(&gameState->vulkan, &gameState->window));
    PS_CHECK(psScenesInit(gameState));
    __psApplicationUpdateFramerateCalculation(gameState);

    // this is for prod
    PS_CHECK(psScenesSwitchWithoutTransition(gameState, PS_SCENE_TYPE_SPLASH));

    memset(&gameState->input, 0, sizeof(PS_Input));

    // // this is for dev/testing
    // if(!psScenesMainMenuInit(gameState)) {
    //     PS_LOG("Failed to switch to loading scene\n");
    //     return false;
    // }
    // if(!psScenesSwitchWithoutTransition(gameState, PS_SCENE_TYPE_MAIN_MENU)) {
    //     PS_LOG("Failed to switch to debug scene\n");
    //     return false;
    // }

    return true;
}

void psApplicationShutdown(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    vkDeviceWaitIdle(gameState->vulkan.device);
    vkQueueWaitIdle(gameState->vulkan.graphicsQueue);
    vkQueueWaitIdle(gameState->vulkan.computeQueue);

    psScenesShutdown(gameState);
    psVulkanShutdown(&gameState->vulkan);
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

    psScenesUpdate(gameState);
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