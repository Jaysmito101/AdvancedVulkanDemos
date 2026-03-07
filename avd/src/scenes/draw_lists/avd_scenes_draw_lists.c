#include "avd_application.h"
#include "scenes/avd_scenes.h"
#include "vulkan/vulkan_core.h"

static AVD_SceneDrawLists *PRIV_avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_DRAW_LISTS);
    return &scene->drawLists;
}

bool avdSceneDrawListsCheckIntegrity(AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // DrawLists scene always passes integrity check
    // as the DrawLists scene assets are directly embedded
    // in the application binary.
    return true;
}

bool avdSceneDrawListsRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneDrawListsCheckIntegrity;
    api->init           = avdSceneDrawListsInit;
    api->render         = avdSceneDrawListsRender;
    api->update         = avdSceneDrawListsUpdate;
    api->destroy        = avdSceneDrawListsDestroy;
    api->load           = avdSceneDrawListsLoad;
    api->inputEvent     = avdSceneDrawListsInputEvent;

    api->id          = "Bloom";
    api->displayName = "Draw Lists";

    return true;
}

bool avdSceneDrawListsInit(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneDrawLists *drawLists = PRIV_avdSceneGetTypePtr(scene);
    AVD_LOG_INFO("Initializing main menu scene");
    drawLists->isDrawListsEnabled = true;

    AVD_CHECK(avdRenderableTextCreate(
        &drawLists->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "DrawLists",
        512.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &drawLists->uiInfoText,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "...",
        24.0f));

    return true;
}

void avdSceneDrawListsDestroy(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneDrawLists *drawLists = PRIV_avdSceneGetTypePtr(scene);

    AVD_LOG_INFO("Destroying DrawLists scene");
    avdRenderableTextDestroy(&drawLists->title, &appState->vulkan);
    avdRenderableTextDestroy(&drawLists->uiInfoText, &appState->vulkan);
}

bool avdSceneDrawListsLoad(AVD_AppState *appState, AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);
    *statusMessage = NULL;
    *progress      = 1.0f;
    return true;
}

void avdSceneDrawListsInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_SceneDrawLists *drawLists = PRIV_avdSceneGetTypePtr(scene);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        }
    }
}

bool avdSceneDrawListsUpdate(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneDrawLists *drawLists = PRIV_avdSceneGetTypePtr(scene);

    const float scale = (float)appState->framerate.deltaTime * 2.0f;
    static char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "DrawLists:\n"
             "General Stats:\n"
             "  - Framerate: %zu FPS\n"
             "  - Frame Time: %.2f ms\n",
             appState->framerate.fps,
             appState->framerate.deltaTime * 1000.0f);
    AVD_CHECK(avdRenderableTextUpdate(&drawLists->uiInfoText,
                                      &appState->fontRenderer,
                                      &appState->vulkan,
                                      buffer));

    return true;
}

bool avdSceneDrawListsRender(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneDrawLists *drawLists = PRIV_avdSceneGetTypePtr(scene);

    AVD_Vulkan *vulkan           = &appState->vulkan;
    AVD_VulkanRenderer *renderer = &appState->renderer;

    VkCommandBuffer commandBuffer = avdVulkanRendererGetCurrentCmdBuffer(&appState->renderer);

    float frameWidth  = (float)renderer->sceneFramebuffer.width;
    float frameHeight = (float)renderer->sceneFramebuffer.height;

    float titleWidth, titleHeight;
    float uiInfoTextWidth, uiInfoTextHeight;

    avdRenderableTextGetSize(&drawLists->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&drawLists->uiInfoText, &uiInfoTextWidth, &uiInfoTextHeight);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));
    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][Scene]:DrawLists/Render");

    avdRenderText(
        vulkan,
        &appState->fontRenderer,
        &drawLists->title,
        commandBuffer,
        (frameWidth - 1330.0f) / 2.0f, (frameHeight + 100.0f) / 2.0f,
        0.8f, 1.2f, 4.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    avdRenderText(
        vulkan,
        &appState->fontRenderer,
        &drawLists->uiInfoText,
        commandBuffer,
        10.0f, 10.0f + uiInfoTextHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);
    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}
