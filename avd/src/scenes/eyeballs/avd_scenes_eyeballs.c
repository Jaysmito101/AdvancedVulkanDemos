#include "scenes/eyeballs/avd_scenes_eyeballs.h"
#include "avd_application.h"

typedef struct {
    AVD_Matrix4x4 viewModelMatrix;
    AVD_Matrix4x4 projectionMatrix;

    int32_t indexOffset;
    int32_t indexCount;
    int32_t pad0;
    int32_t pad1;
} AVD_EyeballPushConstants;

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
    AVD_CHECK(avdEyeballCreate(
        &eyeballs->eyeballs[0],
        &appState->vulkan));
    AVD_VulkanPipelineCreationInfo pipelineCreationInfo = {0};
    avdPipelineUtilsPipelineCreationInfoInit(&pipelineCreationInfo);
    pipelineCreationInfo.enableDepthTest = true;
    pipelineCreationInfo.enableBlend     = false;
    pipelineCreationInfo.cullMode        = VK_CULL_MODE_NONE;
    pipelineCreationInfo.frontFace       = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &eyeballs->pipelineLayout,
        &eyeballs->pipeline,
        appState->vulkan.device,
        &eyeballs->eyeballs[0].descriptorSetLayout,
        1,
        sizeof(AVD_EyeballPushConstants),
        appState->renderer.sceneFramebuffer.renderPass,
        (AVD_UInt32)appState->renderer.sceneFramebuffer.colorAttachments.count,
        "EyeballsSceneVert",
        "EyeballsSceneFrag",
        NULL,
        &pipelineCreationInfo));

    eyeballs->sceneWidth  = (float)appState->renderer.sceneFramebuffer.width;
    eyeballs->sceneHeight = (float)appState->renderer.sceneFramebuffer.height;

    return true;
}

void avdSceneEyeballsDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneEyeballs *eyeballs = __avdSceneGetTypePtr(scene);

    vkDestroyPipeline(appState->vulkan.device, eyeballs->pipeline, NULL);
    vkDestroyPipelineLayout(appState->vulkan.device, eyeballs->pipelineLayout, NULL);

    // Destroy the eyeball
    avdEyeballDestroy(&eyeballs->eyeballs[0], &appState->vulkan);

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

static bool __avdSceneUpdateCamera(AVD_SceneEyeballs *eyeballs, AVD_Float timer)
{
    AVD_ASSERT(eyeballs != NULL);

    timer = timer * 0.1f;
    AVD_Float rad = 8.0f;

    eyeballs->viewModelMatrix = avdMatLookAt(
        avdVec3(rad * sinf(timer), 4.0f, rad * cosf(timer)), // Camera position
        // avdVec3(1.0, 2.0f, 0.0), // Camera position
        avdVec3(0.0f, 0.0f, 0.0f),                             // Look at the origin
        avdVec3(0.0f, 1.0f, 0.0f)                              // Up vector
    );

    eyeballs->projectionMatrix = avdMatPerspective(
        avdDeg2Rad(67.0f),
        eyeballs->sceneWidth / eyeballs->sceneHeight,
        0.1f,
        100.0f);

    return true;
}

bool avdSceneEyeballsUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneEyeballs *eyeballs = __avdSceneGetTypePtr(scene);

    AVD_CHECK(__avdSceneUpdateCamera(eyeballs, (float)appState->framerate.currentTime));

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

    AVD_EyeballPushConstants pushConstants = {
        .viewModelMatrix  = eyeballs->viewModelMatrix,
        .projectionMatrix = eyeballs->projectionMatrix,
        .indexOffset      = eyeballs->eyeballs[0].scleraMesh.indexOffset,
        .indexCount       = eyeballs->eyeballs[0].scleraMesh.triangleCount * 3,
    };
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, eyeballs->pipeline);
    vkCmdPushConstants(commandBuffer, eyeballs->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, eyeballs->pipelineLayout, 0, 1, &eyeballs->eyeballs[0].descriptorSet, 0, NULL);
    vkCmdDraw(commandBuffer, eyeballs->eyeballs[0].scleraMesh.triangleCount * 3, 1, 0, 0);

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