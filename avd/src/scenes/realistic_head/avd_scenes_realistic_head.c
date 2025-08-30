#include "scenes/realistic_head/avd_scenes_realistic_head.h"
#include "avd_application.h"

typedef struct {
    AVD_Matrix4x4 viewModelMatrix;
    AVD_Matrix4x4 projectionMatrix;

    int32_t indexOffset;
    int32_t indexCount;
    int32_t pad0;
    int32_t pad1;
} AVD_RealisticHeadPushConstants;

static AVD_SceneRealisticHead *__avdSceneGetTypePtr(union AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_REALISTIC_HEAD);
    return &scene->realisticHead;
}

bool avdSceneRealisticHeadCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // realisticHead scene always passes integrity check
    // as the realisticHead scene assets are directly embedded
    // in the application binary.
    return true;
}

bool avdSceneRealisticHeadRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneRealisticHeadCheckIntegrity;
    api->init           = avdSceneRealisticHeadInit;
    api->render         = avdSceneRealisticHeadRender;
    api->update         = avdSceneRealisticHeadUpdate;
    api->destroy        = avdSceneRealisticHeadDestroy;
    api->load           = avdSceneRealisticHeadLoad;
    api->inputEvent     = avdSceneRealisticHeadInputEvent;

    api->displayName = "realisticHead";
    api->id          = "StaticSubsurfaceScattering";

    return true;
}

bool avdSceneRealisticHeadInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    // Initialize title and info text
    AVD_CHECK(avdRenderableTextCreate(
        &realisticHead->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "realisticHead",
        64.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &realisticHead->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "This scene demonstrates the rendering of realisticHead with subsurface scattering.",
        24.0f));

    realisticHead->sceneWidth  = (float)appState->renderer.sceneFramebuffer.width;
    realisticHead->sceneHeight = (float)appState->renderer.sceneFramebuffer.height;

    return true;
}

void avdSceneRealisticHeadDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    vkDestroyPipeline(appState->vulkan.device, realisticHead->pipeline, NULL);
    vkDestroyPipelineLayout(appState->vulkan.device, realisticHead->pipelineLayout, NULL);

    avdRenderableTextDestroy(&realisticHead->title, &appState->vulkan);
    avdRenderableTextDestroy(&realisticHead->info, &appState->vulkan);
}

bool avdSceneRealisticHeadLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    *progress      = 1.0f;
    *statusMessage = "realisticHead scene loaded successfully.";
    return true;
}

void avdSceneRealisticHeadInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
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

static bool __avdSceneUpdateCamera(AVD_SceneRealisticHead *realisticHead, AVD_Float timer)
{
    AVD_ASSERT(realisticHead != NULL);

    timer         = timer * 0.1f;
    AVD_Float rad = 8.0f;

    realisticHead->viewModelMatrix = avdMatLookAt(
        avdVec3(rad * sinf(timer), 4.0f, rad * cosf(timer)), // Camera position
        // avdVec3(1.0, 2.0f, 0.0), // Camera position
        avdVec3(0.0f, 0.0f, 0.0f), // Look at the origin
        avdVec3(0.0f, 1.0f, 0.0f)  // Up vector
    );

    realisticHead->projectionMatrix = avdMatPerspective(
        avdDeg2Rad(67.0f),
        realisticHead->sceneWidth / realisticHead->sceneHeight,
        0.1f,
        100.0f);

    return true;
}

bool avdSceneRealisticHeadUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    AVD_CHECK(__avdSceneUpdateCamera(realisticHead, (float)appState->framerate.currentTime));

    return true;
}

bool avdSceneRealisticHeadRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    VkCommandBuffer commandBuffer         = appState->renderer.resources[appState->renderer.currentFrameIndex].commandBuffer;
    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&realisticHead->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&realisticHead->info, &infoWidth, &infoHeight);

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &realisticHead->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &realisticHead->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}