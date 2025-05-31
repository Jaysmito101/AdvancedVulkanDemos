#include "scenes/2d_radiance_cascades/avd_scenes_2d_radiance_cascades.h"
#include "scenes/avd_scenes.h"

static AVD_Scene2DRadianceCascades *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_2D_RADIANCE_CASCADES);
    return &scene->radianceCascades;
}

bool avdScene2DRadianceCascadesInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);


    return true;
}


bool avdScene2DRadianceCascadesRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    
    AVD_Scene2DRadianceCascades *radianceCascades = __avdSceneGetTypePtr(scene);


    return true;
}

bool avdScene2DRadianceCascadesUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    // Update logic for the 2D Radiance Cascades scene
    // Currently, no specific update logic is implemented.
    return true;
}

void avdScene2DRadianceCascadesDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);


}

bool avdScene2DRadianceCascadesLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);
    *statusMessage = NULL;
    *progress      = 1.0f;

    // The 2D Radiance Cascades scene does not have any external dependencies
    // and is always considered to be in a valid state.
    return true;
}

void avdScene2DRadianceCascadesInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    // Handle input events for the 2D Radiance Cascades scene
    // Currently, no specific input handling is implemented.
}

bool avdScene2DRadianceCascadesCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // The 2D Radiance Cascades scene does not have any external dependencies
    // and is always considered to be in a valid state.
    return true;
}

bool avdScene2DRadianceCascadesRegisterApi(AVD_SceneAPI *api) 
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdScene2DRadianceCascadesCheckIntegrity;
    api->init           = avdScene2DRadianceCascadesInit;
    api->render         = avdScene2DRadianceCascadesRender;
    api->update         = avdScene2DRadianceCascadesUpdate;
    api->destroy        = avdScene2DRadianceCascadesDestroy;
    api->load           = avdScene2DRadianceCascadesLoad;
    api->inputEvent     = avdScene2DRadianceCascadesInputEvent;

    api->displayName    = "2D GI (Radiance Cascades)";
    api->id             = "DDGIPlaceholder";

    return true;
}
