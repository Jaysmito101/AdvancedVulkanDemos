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

    subsurfaceScattering->loadStage = 0;
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

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    switch (subsurfaceScattering->loadStage) {
        case 0:
            *statusMessage = "Loading Alien Model";
            AVD_CHECK(avd3DSceneLoadObj("assets/scene_subsurface_scattering/alien.obj", &subsurfaceScattering->models, AVD_OBJ_LOAD_FLAG_NONE));
            break;
        case 1:
            *statusMessage = "Loading Buddha Model";
            AVD_CHECK(avd3DSceneLoadObj("assets/scene_subsurface_scattering/buddha.obj", &subsurfaceScattering->models, AVD_OBJ_LOAD_FLAG_NONE));
            break;
        case 2:
            *statusMessage = "Loading Standford Dragon Model";
            AVD_CHECK(avd3DSceneLoadObj("assets/scene_subsurface_scattering/standford_dragon.obj", &subsurfaceScattering->models, AVD_OBJ_LOAD_FLAG_NONE));
            break;
        case 3:
            *statusMessage = "Loading Alien Thickness Map";
            break;
        case 4:
            *statusMessage = "Loading Buddha Thickness Map";
            break;
        case 5:
            *statusMessage = "Loading Standford Dragon Thickness Map";
            break;
        case 6:
            *statusMessage = NULL;
            *progress      = 1.0f;
            subsurfaceScattering->loadStage = 0; // Reset load stage for next load
            AVD_LOG("Subsurface Scattering scene loaded successfully.\n");
            avd3DSceneDebugLog(&subsurfaceScattering->models, "SubsurfaceScattering/Models");
            return true;
        default:
            AVD_LOG("Subsurface Scattering scene load stage is invalid: %d\n", subsurfaceScattering->loadStage);
            return false;
    }

    subsurfaceScattering->loadStage++;
    *progress = (float)subsurfaceScattering->loadStage / 6.0f; // Update progress based on load stage
    return false; // Continue loading
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
