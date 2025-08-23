#include "scenes/deccer_cubes/avd_scenes_deccer_cubes.h"
#include "scenes/avd_scenes.h"
#include "avd_application.h"

static AVD_SceneDeccerCubes *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_DECCER_CUBES);
    return &scene->deccerCubes;
}


bool avdSceneDeccerCubesCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // Not really a dependency but we should ensure that its present
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/LICENSE");

    // The Model files
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/SM_Deccer_Cubes.glb");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/SM_Deccer_Cubes_Colored.glb");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/SM_Deccer_Cubes_Textured.glb");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/SM_Deccer_Cubes_Textured_Animated.glb");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/SM_Deccer_Cubes_With_Rotation.glb");

    // The images
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Atlas.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Blue_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_LightGreen_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Orange_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Purple_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Red_D.png");

    return true;
}

bool avdSceneDeccerCubesRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneDeccerCubesCheckIntegrity;
    api->init           = avdSceneDeccerCubesInit;
    api->render         = avdSceneDeccerCubesRender;
    api->update         = avdSceneDeccerCubesUpdate;
    api->destroy        = avdSceneDeccerCubesDestroy;
    api->load           = avdSceneDeccerCubesLoad;
    api->inputEvent     = avdSceneDeccerCubesInputEvent;

    api->displayName = "Deccer Cubes";
    api->id          = "DDGIPlaceholder";

    return true;
}


bool avdSceneDeccerCubesInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneDeccerCubes* deccerCubes = __avdSceneGetTypePtr(scene);

    avd3DSceneCreate(&deccerCubes->scene);

    AVD_CHECK(avdRenderableTextCreate(
        &deccerCubes->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Deccer Cubes Demo",
        48.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &deccerCubes->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "A simple GLTF scene with some weird transforms to test the transform calculation edge cases",
        18.0f));

    return true;
}

void avdSceneDeccerCubesDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneDeccerCubes* deccerCubes = __avdSceneGetTypePtr(scene);

    avd3DSceneDestroy(&deccerCubes->scene);
    avdRenderableTextDestroy(&deccerCubes->title, &appState->vulkan);
    avdRenderableTextDestroy(&deccerCubes->info, &appState->vulkan);
}

bool avdSceneDeccerCubesLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);
    *statusMessage = NULL;
    *progress      = 1.0f;

    // The Deccer Cubes scene does not have any external dependencies
    // and is always considered to be in a valid state.
    return true;
}

void avdSceneDeccerCubesInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        }
    }
}

bool avdSceneDeccerCubesRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneDeccerCubes *deccerCubes = __avdSceneGetTypePtr(scene);

    VkCommandBuffer commandBuffer = appState->renderer.resources[appState->renderer.currentFrameIndex].commandBuffer;
    
    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&deccerCubes->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&deccerCubes->info, &infoWidth, &infoHeight);

    
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &deccerCubes->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &deccerCubes->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));



    return true;
}

bool avdSceneDeccerCubesUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    // Update logic for the Deccer Cubes scene
    // Currently, no specific update logic is implemented.
    return true;
}
