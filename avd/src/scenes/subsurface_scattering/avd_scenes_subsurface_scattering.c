#include "scenes/subsurface_scattering/avd_scenes_subsurface_scattering.h"
#include "scenes/avd_scenes.h"

static AVD_SceneSubsurfaceScattering *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_SUBSURFACE_SCATTERING);
    return &scene->subsurfaceScattering;
}

bool avdSceneSubsurfaceScatteringInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    avd3DSceneCreate(&subsurfaceScattering->models);

    return true;
}


bool avdSceneSubsurfaceScatteringRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);


    return true;
}

bool avdSceneSubsurfaceScatteringUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    // Update logic for the Subsurface Scattering scene
    // Currently, no specific update logic is implemented.
    return true;
}

void avdSceneSubsurfaceScatteringDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);
    avd3DSceneDestroy(&subsurfaceScattering->models);
}

bool avdSceneSubsurfaceScatteringLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);
    *statusMessage = NULL;
    *progress      = 1.1f;

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    AVD_CHECK(avd3DSceneLoadObj("assets/scene_subsurface_scattering/alien.obj", &subsurfaceScattering->models));

    return true;
}

void avdSceneSubsurfaceScatteringInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    // Handle input events for the Subsurface Scattering scene
    // Currently, no specific input handling is implemented.
}

bool avdSceneSubsurfaceScatteringCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // Not really a dependency but we should ensure that its present
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/alien.obj");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/alien_thickness_map.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/buddha.obj");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/buddha_thickness_map.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/standford_dragon.obj");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/standford_dragon_thickness_map.png");
    return true;
}

bool avdSceneSubsurfaceScatteringRegisterApi(AVD_SceneAPI *api) 
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneSubsurfaceScatteringCheckIntegrity;
    api->init           = avdSceneSubsurfaceScatteringInit;
    api->render         = avdSceneSubsurfaceScatteringRender;
    api->update         = avdSceneSubsurfaceScatteringUpdate;
    api->destroy        = avdSceneSubsurfaceScatteringDestroy;
    api->load           = avdSceneSubsurfaceScatteringLoad;
    api->inputEvent     = avdSceneSubsurfaceScatteringInputEvent;

    api->displayName    = "Subsurface Scattering";
    api->id             = "DDGIPlaceholder";

    return true;
}
