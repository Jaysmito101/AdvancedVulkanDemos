#include "avd_application.h"

static void __avdApplicationUpdateFramerateCalculation(AVD_Frametime *framerateInfo)
{
    AVD_ASSERT(framerateInfo != NULL);

    double currentTime         = glfwGetTime();
    framerateInfo->currentTime = currentTime;
    framerateInfo->deltaTime   = currentTime - framerateInfo->lastTime;
    framerateInfo->lastTime    = currentTime;
    framerateInfo->lastSecondFrameCounter++;
    framerateInfo->instanteneousFrameRate = (size_t)(1.0 / framerateInfo->deltaTime);

    if (currentTime - framerateInfo->lastSecondTime >= 1.0) {
        framerateInfo->fps                    = framerateInfo->lastSecondFrameCounter;
        framerateInfo->lastSecondFrameCounter = 0;
        framerateInfo->lastSecondTime         = currentTime;
    }
}

bool avdApplicationInit(AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);

    appState->running = true;

    AVD_CHECK(avdWindowInit(&appState->window, appState));
    AVD_CHECK(avdVulkanInit(&appState->vulkan, &appState->window, &appState->surface));
    AVD_CHECK(avdVulkanSwapchainCreate(&appState->swapchain, &appState->vulkan, appState->surface, &appState->window));
    AVD_CHECK(avdVulkanRendererCreate(&appState->renderer, &appState->vulkan, &appState->swapchain, GAME_WIDTH, GAME_HEIGHT));
    AVD_CHECK(avdFontRendererInit(&appState->fontRenderer, &appState->vulkan, &appState->renderer));
    AVD_CHECK(avdFontRendererAddBasicFonts(&appState->fontRenderer));
    AVD_CHECK(avdVulkanPresentationInit(&appState->presentation, &appState->vulkan, &appState->swapchain, &appState->fontRenderer));
    AVD_CHECK(avdUiInit(&appState->ui, appState));
    AVD_CHECK(avdSceneManagerInit(&appState->sceneManager, appState));

    __avdApplicationUpdateFramerateCalculation(&appState->framerate);

    memset(&appState->input, 0, sizeof(AVD_Input));

    return true;
}

void avdApplicationShutdown(AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);

    avdVulkanWaitIdle(&appState->vulkan);

    avdSceneManagerDestroy(&appState->sceneManager, appState);
    avdUiDestroy(&appState->ui, appState);
    avdVulkanPresentationDestroy(&appState->presentation, &appState->vulkan);
    avdFontRendererShutdown(&appState->fontRenderer);
    avdVulkanRendererDestroy(&appState->renderer, &appState->vulkan);
    avdVulkanSwapchainDestroy(&appState->swapchain, &appState->vulkan);
    avdVulkanDestroySurface(&appState->vulkan, appState->surface);
    avdVulkanShutdown(&appState->vulkan);
    avdWindowShutdown(&appState->window);
}

bool avdApplicationIsRunning(AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    return appState->running;
}

void avdApplicationUpdate(AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);

    __avdApplicationUpdateFramerateCalculation(&appState->framerate);

    avdInputNewFrame(&appState->input);
    avdWindowPollEvents();
    avdInputCalculateDeltas(&appState->input);
    avdApplicationUpdateWithoutPolling(appState);

    // update the title of window with stats
    static char title[256];
    AVD_Frametime *framerateInfo = &appState->framerate;
    snprintf(title, sizeof(title), "Advanced Vulkan Demos -- FPS(Stable): %zu, FPS(Instant): %zu, DeltaTime: %.3f", framerateInfo->fps, framerateInfo->instanteneousFrameRate, framerateInfo->deltaTime);
    glfwSetWindowTitle(appState->window.window, title);
}

void avdApplicationUpdateWithoutPolling(AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    avdSceneManagerUpdate(&appState->sceneManager, appState);
    avdApplicationRender(appState);
}

void avdApplicationRender(AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);

    if (appState->window.isMinimized) {
        // sleep to avoid busy waiting
        avdSleep(100);
        return;
    }

    if (avdVulkanSwapchainRecreateIfNeeded(&appState->swapchain, &appState->vulkan, &appState->window)) {
        if (!avdVulkanRendererRecreateResources(&appState->renderer, &appState->vulkan, &appState->swapchain))
            AVD_LOG("Failed to recreate Vulkan renderer resources\n");
        return; // skip this frame
    }

    if (!avdVulkanRendererBegin(&appState->renderer, &appState->vulkan, &appState->swapchain)) {
        AVD_LOG("Failed to begin Vulkan renderer\n");
        return; // do not render this frame
    }

    if (!avdSceneManagerRender(&appState->sceneManager, appState)) {
        AVD_LOG("Failed to render scene\n");
        if (!avdVulkanRendererCancelFrame(&appState->renderer, &appState->vulkan))
            AVD_LOG("Failed to cancel Vulkan renderer frame\n");
        return; // do not render this frame
    }

    if (!avdVulkanPresentationRender(&appState->presentation, &appState->vulkan, &appState->renderer, &appState->swapchain, &appState->sceneManager, &appState->fontRenderer, appState->renderer.currentImageIndex)) {
        if (!avdVulkanRendererCancelFrame(&appState->renderer, &appState->vulkan))
            AVD_LOG("Failed to cancel Vulkan renderer frame\n");
        return; // do not render this frame
    }

    if (!avdVulkanRendererEnd(&appState->renderer, &appState->vulkan, &appState->swapchain)) {
        // Nothing to do here for now...
    }
}