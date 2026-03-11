#include "scenes/gui/avd_scenes_gui.h"
#include "avd_application.h"
#include "common/avd_drawlist.h"
#include "scenes/avd_scenes.h"

static AVD_SceneImmediateGui *PRIV_avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_GUI);
    return &scene->gui;
}

bool avdSceneImmediateGuiCheckIntegrity(AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // Bloom scene always passes integrity check
    // as the bloom scene assets are directly embedded
    // in the application binary.
    return true;
}

bool avdSceneImmediateGuiRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneImmediateGuiCheckIntegrity;
    api->init           = avdSceneImmediateGuiInit;
    api->render         = avdSceneImmediateGuiRender;
    api->update         = avdSceneImmediateGuiUpdate;
    api->destroy        = avdSceneImmediateGuiDestroy;
    api->load           = avdSceneImmediateGuiLoad;
    api->inputEvent     = avdSceneImmediateGuiInputEvent;

    api->id          = "Bloom";
    api->displayName = "Immediate Mode GUI Demo";

    return true;
}

bool avdSceneImmediateGuiInit(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneImmediateGui *gui = PRIV_avdSceneGetTypePtr(scene);
    AVD_LOG_INFO("Initializing immediate GUI scene");

    AVD_CHECK(avdRenderableTextCreate(
        &gui->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Bloom",
        512.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &gui->uiInfoText,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "...",
        24.0f));

    AVD_CHECK(avdDrawListRendererCreate(
        &gui->drawListRenderer,
        &appState->renderer.sceneFramebuffer,
        &appState->vulkan));

    return true;
}

void avdSceneImmediateGuiDestroy(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneImmediateGui *gui = PRIV_avdSceneGetTypePtr(scene);

    AVD_LOG_INFO("Destroying immediate GUI scene");
    avdRenderableTextDestroy(&gui->title, &appState->vulkan);
    avdRenderableTextDestroy(&gui->uiInfoText, &appState->vulkan);
    avdDrawListRendererDestroy(&gui->drawListRenderer, &appState->vulkan);
}

bool avdSceneImmediateGuiLoad(AVD_AppState *appState, AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);
    *statusMessage = NULL;
    *progress      = 1.0f;
    return true;
}

void avdSceneImmediateGuiInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_SceneImmediateGui *gui = PRIV_avdSceneGetTypePtr(scene);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        }
    }
}

bool avdSceneImmediateGuiUpdate(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneImmediateGui *gui = PRIV_avdSceneGetTypePtr(scene);

    const float scale = (float)appState->framerate.deltaTime * 2.0f;

    static char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "Gui:\n"
             "General Stats:\n"
             "  - Framerate: %zu FPS\n"
             "  - Frame Time: %.2f ms\n",
             appState->framerate.fps,
             appState->framerate.deltaTime * 1000.0f);
    AVD_CHECK(avdRenderableTextUpdate(&gui->uiInfoText,
                                      &appState->fontRenderer,
                                      &appState->vulkan,
                                      buffer));

    return true;
}

bool avdSceneImmediateGuiRender(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneImmediateGui *gui = PRIV_avdSceneGetTypePtr(scene);

    AVD_Vulkan *vulkan           = &appState->vulkan;
    AVD_VulkanRenderer *renderer = &appState->renderer;

    VkCommandBuffer commandBuffer = avdVulkanRendererGetCurrentCmdBuffer(&appState->renderer);

    float frameWidth  = (float)renderer->sceneFramebuffer.width;
    float frameHeight = (float)renderer->sceneFramebuffer.height;

    float titleWidth, titleHeight;
    float uiInfoTextWidth, uiInfoTextHeight;

    avdRenderableTextGetSize(&gui->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&gui->uiInfoText, &uiInfoTextWidth, &uiInfoTextHeight);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));
    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][Scene]:Bloom/Render");

    avdRenderText(
        vulkan,
        &appState->fontRenderer,
        &gui->title,
        commandBuffer,
        (frameWidth - 1330.0f) / 2.0f, (frameHeight + 100.0f) / 2.0f,
        0.8f, 1.2f, 4.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    avdRenderText(
        vulkan,
        &appState->fontRenderer,
        &gui->uiInfoText,
        commandBuffer,
        10.0f, 10.0f + uiInfoTextHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    AVD_CHECK(avdDrawListRendererRender(&gui->drawListRenderer, commandBuffer, gui->title.font->fontDescriptorSet));

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);
    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}
