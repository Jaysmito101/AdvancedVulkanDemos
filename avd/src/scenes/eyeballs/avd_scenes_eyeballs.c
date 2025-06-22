#include "avd_application.h"
#include "scenes/eyeballs/avd_scenes_eyeballs.h"

static AVD_SceneEyeballs *__avdSceneGetTypePtr(union AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_EYEBALLS);
    return &scene->eyeballs;
}

bool avdSceneEyeballsCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // Eyeballs scene always passes integrity check
    // as the eyeballs scene assets are directly embedded
    // in the application binary.
    return true;
}

bool avdSceneEyeballsRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneEyeballsCheckIntegrity;
    api->init           = avdSceneEyeballsInit;
    api->render         = avdSceneEyeballsRender;
    api->update         = avdSceneEyeballsUpdate;
    api->destroy        = avdSceneEyeballsDestroy;
    api->load           = avdSceneEyeballsLoad;
    api->inputEvent     = avdSceneEyeballsInputEvent;

    api->displayName = "Eyeballs";
    api->id          = "StaticSubsurfaceScattering";

    return true;
}

bool avdSceneEyeballsInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneEyeballs *eyeballs = __avdSceneGetTypePtr(scene);
    eyeballs->type              = AVD_SCENE_TYPE_EYEBALLS;

    // Initialize title and info text
    AVD_CHECK(avdRenderableTextCreate(
        &eyeballs->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Eyeballs",
        64.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &eyeballs->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "This scene demonstrates the rendering of eyeballs with subsurface scattering.",
        24.0f));

    return true;
}

void avdSceneEyeballsDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneEyeballs *eyeballs = __avdSceneGetTypePtr(scene);

    avdRenderableTextDestroy(&eyeballs->title, &appState->vulkan);
    avdRenderableTextDestroy(&eyeballs->info, &appState->vulkan);
}

bool avdSceneEyeballsLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    *progress      = 1.0f;
    *statusMessage = "Eyeballs scene loaded successfully.";
    return true;
}

void avdSceneEyeballsInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(event != NULL);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        }
    }
}

bool avdSceneEyeballsUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    // Eyeballs scene does not require any specific update logic
    // for now, so we can return true directly.
    return true;
}

bool avdSceneEyeballsRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    VkCommandBuffer commandBuffer = appState->renderer.resources[appState->renderer.currentFrameIndex].commandBuffer;
    AVD_SceneEyeballs *eyeballs   = __avdSceneGetTypePtr(scene);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&eyeballs->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&eyeballs->info, &infoWidth, &infoHeight);

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &eyeballs->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &eyeballs->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}