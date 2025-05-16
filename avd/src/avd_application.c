#include "avd_application.h"
#include "avd_window.h"
#include "avd_vulkan.h"
#include "avd_font_renderer.h"

#include "glfw/glfw3.h"

static void __avdApplicationUpdateFramerateCalculation(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);

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

bool avdApplicationInit(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);

    gameState->running = true;

    AVD_CHECK(avdWindowInit(&gameState->window, gameState));
    AVD_CHECK(avdVulkanInit(&gameState->vulkan, &gameState->window));
    AVD_CHECK(avdScenesInit(gameState));
    AVD_CHECK(avdBloomCreate(&gameState->bloom, &gameState->vulkan, GAME_WIDTH, GAME_HEIGHT));
    AVD_CHECK(avdFontRendererInit(&gameState->fontRenderer, &gameState->vulkan));
    AVD_CHECK(avdFontRendererAddBasicFonts(&gameState->fontRenderer));

    __avdApplicationUpdateFramerateCalculation(gameState);

    memset(&gameState->input, 0, sizeof(AVD_Input));

    // this is for prod
    // AVD_CHECK(avdScenesSwitchWithoutTransition(gameState, AVD_SCENE_TYPE_SPLASH));

    // this is for dev/testing
    AVD_CHECK(avdScenesMainMenuInit(gameState));
    AVD_CHECK(avdScenesSwitchWithoutTransition(gameState, AVD_SCENE_TYPE_MAIN_MENU));

    return true;
}

void avdApplicationShutdown(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);

    vkDeviceWaitIdle(gameState->vulkan.device);
    vkQueueWaitIdle(gameState->vulkan.graphicsQueue);
    vkQueueWaitIdle(gameState->vulkan.computeQueue);

    avdFontRendererShutdown(&gameState->fontRenderer);
    avdBloomDestroy(&gameState->bloom, &gameState->vulkan);
    avdScenesShutdown(gameState);
    avdVulkanShutdown(&gameState->vulkan);
    avdWindowShutdown(&gameState->window);
}

bool avdApplicationIsRunning(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);
    return gameState->running;
}

void avdApplicationUpdate(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);

    __avdApplicationUpdateFramerateCalculation(gameState);

    avdInputNewFrame(&gameState->input);
    avdWindowPollEvents();
    avdInputCalculateDeltas(&gameState->input);
    avdApplicationUpdateWithoutPolling(gameState);
}



void avdApplicationUpdateWithoutPolling(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);

    avdScenesUpdate(gameState);
    avdApplicationRender(gameState);
}


void avdApplicationRender(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);

    // to not render if the window is minimized
    if (gameState->window.isMinimized) {
        return;
    }

    avdVulkanRendererRender(gameState);
}