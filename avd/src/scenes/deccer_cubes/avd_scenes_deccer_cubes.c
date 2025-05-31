#include "scenes/deccer_cubes/avd_scenes_deccer_cubes.h"
#include "scenes/avd_scenes.h"

static AVD_SceneDeccerCubes *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_DECCER_CUBES);
    return &scene->deccerCubes;
}

bool avdSceneDeccerCubesInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);


    return true;
}


bool avdSceneDeccerCubesRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneDeccerCubes *deccerCubes = __avdSceneGetTypePtr(scene);


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

void avdSceneDeccerCubesDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);


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

    // Handle input events for the Deccer Cubes scene
    // Currently, no specific input handling is implemented.
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

    api->displayName    = "Deccer Cubes";
    api->id             = "DDGIPlaceholder";

    return true;
}
