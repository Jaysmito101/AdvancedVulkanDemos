#include "scenes/avd_scenes.h"
#include "avd_application.h"

static AVD_SceneMainMenu *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_MAIN_MENU);
    return &scene->mainMenu;
}

bool avdSceneMainMenuCheckIntegrity(AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    // Main menu always passes integrity check
    // as the main menu assets are directly embedded
    // in the application binary.
    return true;
}

bool avdSceneMainMenuRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneMainMenuCheckIntegrity;
    api->init = avdSceneMainMenuInit;
    api->render = avdSceneMainMenuRender;
    api->update = avdSceneMainMenuUpdate;
    api->destroy = avdSceneMainMenuDestroy;
    api->load = avdSceneMainMenuLoad;
    api->inputEvent = avdSceneMainMenuInputEvent;

    return true;
}

bool avdSceneMainMenuInit(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);
    AVD_LOG("Initializing main menu scene\n");
    mainMenu->loadingCount = 0;

    AVD_CHECK(avdRenderableTextCreate(
        &mainMenu->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Advanced Vulkan Demos",
        72.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &mainMenu->creditsText,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Made with Love by Jaysmito Mukherjee",
        36.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &mainMenu->githubLinkText,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "(https://github.com/Jaysmito101/AdvancedVulkanDemos)",
        16.0f));

    return true;
}

void avdSceneMainMenuDestroy(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);

    AVD_LOG("Destroying main menu scene\n");
    avdRenderableTextDestroy(&mainMenu->title, &appState->vulkan);
    avdRenderableTextDestroy(&mainMenu->creditsText, &appState->vulkan);
    avdRenderableTextDestroy(&mainMenu->githubLinkText, &appState->vulkan);
}

bool avdSceneMainMenuLoad(AVD_AppState *appState, AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);
    AVD_LOG("Loading main menu scene\n");
    // nothing to load really here but some busy waiting
    if (mainMenu->loadingCount < 4)
    {
        mainMenu->loadingCount++;
        static char buffer[256];
        snprintf(buffer, sizeof(buffer), "Loading main menu scene: %d%%", mainMenu->loadingCount * 100 / 4);
        AVD_LOG("%s\n", buffer);
        *statusMessage = buffer;
        *progress = (float)mainMenu->loadingCount / 4.0f;
        avdSleep(100);
        return false;
    }

    *statusMessage = NULL;
    *progress = 1.0f;

    return true;
}

void avdSceneMainMenuInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    // AVD_LOG("Main menu input event: %s\n", avdInputEventTypeToString(event->type));
}

bool avdSceneMainMenuUpdate(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);
    return true;
}

bool avdSceneMainMenuRender(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);

    AVD_Vulkan *vulkan = &appState->vulkan;
    AVD_VulkanRenderer *renderer = &appState->renderer;

    uint32_t currentFrameIndex = renderer->currentFrameIndex;
    VkCommandBuffer commandBuffer = renderer->resources[currentFrameIndex].commandBuffer;

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    // AVD_LOG("Rendering main menu scene\n");

    float titleWidth, titleHeight;
    float creditsWidth, creditsHeight;
    float githubLinkWidth, githubLinkHeight;
    {
        // render the title
        avdRenderableTextGetSize(&mainMenu->title, &titleWidth, &titleHeight);
        avdRenderableTextGetSize(&mainMenu->creditsText, &creditsWidth, &creditsHeight);
        avdRenderableTextGetSize(&mainMenu->githubLinkText, &githubLinkWidth, &githubLinkHeight);

        avdRenderText(
            &appState->vulkan,
            &appState->fontRenderer,
            &mainMenu->title,
            commandBuffer,
            ((float)renderer->sceneFramebuffer.width - titleWidth) / 2.0f, titleHeight,
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            renderer->sceneFramebuffer.width,
            renderer->sceneFramebuffer.height);

        avdRenderText(
            &appState->vulkan,
            &appState->fontRenderer,
            &mainMenu->creditsText,
            commandBuffer,
            ((float)renderer->sceneFramebuffer.width - creditsWidth) / 2.0f, titleHeight + creditsHeight + 10.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            renderer->sceneFramebuffer.width,
            renderer->sceneFramebuffer.height);

        avdRenderText(
            &appState->vulkan,
            &appState->fontRenderer,
            &mainMenu->githubLinkText,
            commandBuffer,
            ((float)renderer->sceneFramebuffer.width - githubLinkWidth) / 2.0f, titleHeight + creditsHeight + 10.0f + githubLinkHeight + 10.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            renderer->sceneFramebuffer.width,
            renderer->sceneFramebuffer.height);
    }

    float minX = 0.0f;
    float minY = (titleHeight + creditsHeight + githubLinkHeight + 40.0f);

    avdUiBegin(
        commandBuffer,
        &appState->ui,
        appState,
        (float)renderer->sceneFramebuffer.width,
        (float)renderer->sceneFramebuffer.height - minY,
        0.0f, minY,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    avdUiDrawRect(
        commandBuffer,
        &appState->ui,
        appState,
        0.0f, 0.0f,
        100.0, 100.0,
        0.0f, 1.0f, 0.0f, 1.0f);

    avdUiEnd(
        commandBuffer,
        &appState->ui,
        appState);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));
    return true;
}