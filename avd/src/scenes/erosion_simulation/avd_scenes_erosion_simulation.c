#include "scenes/erosion_simulation/avd_scenes_erosion_simulation.h"

#include "avd_application.h"
#include "scenes/avd_scenes.h"

static AVD_SceneErosionSimulation *PRIV_avdSceneErosionSimulationGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_EROSION_SIMULATION);
    return &scene->erosionSimulation;
}

bool avdSceneErosionSimulationCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;
    return true;
}

bool avdSceneErosionSimulationRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneErosionSimulationCheckIntegrity;
    api->init           = avdSceneErosionSimulationInit;
    api->render         = avdSceneErosionSimulationRender;
    api->update         = avdSceneErosionSimulationUpdate;
    api->destroy        = avdSceneErosionSimulationDestroy;
    api->load           = avdSceneErosionSimulationLoad;
    api->inputEvent     = avdSceneErosionSimulationInputEvent;

    api->displayName    = "Erosion Simulation";
    api->id             = "ErosionSimulation";
    api->thumbnailAsset = "DeccerCubes";

    return true;
}

bool avdSceneErosionSimulationInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneErosionSimulation *erosionSimulation = PRIV_avdSceneErosionSimulationGetTypePtr(scene);

    AVD_CHECK(avdRenderableTextCreate(
        &erosionSimulation->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Erosion Simulation",
        64.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &erosionSimulation->status,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Heightmap erosion scene scaffold.\n"
        "Terrain generation and simulation controls can be added here.\n\n"
        "Press Esc to return to the scene menu.",
        24.0f));

    return true;
}

void avdSceneErosionSimulationDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneErosionSimulation *erosionSimulation = PRIV_avdSceneErosionSimulationGetTypePtr(scene);
    avdRenderableTextDestroy(&erosionSimulation->title, &appState->vulkan);
    avdRenderableTextDestroy(&erosionSimulation->status, &appState->vulkan);
}

bool avdSceneErosionSimulationLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    *statusMessage = NULL;
    *progress      = 1.0f;
    return true;
}

void avdSceneErosionSimulationInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(event != NULL);

    if (event->type == AVD_INPUT_EVENT_KEY && event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
        avdSceneManagerSwitchToScene(
            &appState->sceneManager,
            AVD_SCENE_TYPE_MAIN_MENU,
            appState);
    }
}

bool avdSceneErosionSimulationUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    return true;
}

bool avdSceneErosionSimulationRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneErosionSimulation *erosionSimulation = PRIV_avdSceneErosionSimulationGetTypePtr(scene);
    AVD_VulkanRenderer *renderer                  = &appState->renderer;
    VkCommandBuffer commandBuffer                 = avdVulkanRendererGetCurrentCmdBuffer(renderer);
    const float frameWidth                        = (float)renderer->sceneFramebuffer.width;
    const float frameHeight                       = (float)renderer->sceneFramebuffer.height;

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, renderer));

    float titleWidth, titleHeight;
    float statusWidth, statusHeight;
    avdRenderableTextGetSize(&erosionSimulation->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&erosionSimulation->status, &statusWidth, &statusHeight);

    const float contentHeight = titleHeight + statusHeight + 48.0f;
    const float titleY        = (frameHeight - contentHeight) * 0.5f;
    const float statusY       = titleY + titleHeight + 48.0f;

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &erosionSimulation->title,
        commandBuffer,
        (frameWidth - titleWidth) * 0.5f,
        titleY,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &erosionSimulation->status,
        commandBuffer,
        (frameWidth - statusWidth) * 0.5f,
        statusY,
        1.0f, 0.85f, 0.65f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));
    return true;
}
