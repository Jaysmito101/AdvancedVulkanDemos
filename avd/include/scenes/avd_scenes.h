#ifndef AVD_SCENES_H
#define AVD_SCENES_H

#include "scenes/avd_scenes_base.h"
#include "scenes/avd_scenes_main_menu.h"
#include "scenes/bloom/avd_scenes_bloom.h"
#include "scenes/2d_radiance_cascades/avd_scenes_2d_radiance_cascades.h"
#include "scenes/deccer_cubes/avd_scenes_deccer_cubes.h"
#include "scenes/subsurface_scattering/avd_scenes_subsurface_scattering.h"

typedef union AVD_Scene {
    AVD_SceneType type;
    AVD_SceneMainMenu mainMenu;
    AVD_SceneBloom bloom;
    AVD_Scene2DRadianceCascades radianceCascades;
    AVD_SceneDeccerCubes deccerCubes;
    AVD_SceneSubsurfaceScattering subsurfaceScattering;
} AVD_Scene;

typedef struct AVD_SceneManager {
    AVD_SceneAPI api[AVD_SCENE_TYPE_COUNT];
    AVD_Scene scene;
    AVD_SceneType currentSceneType;
    bool isSceneLoaded;
    bool isSceneInitialized;

    float sceneLoadingProgress;
    const char *sceneLoadingStatusMessage;

    bool sceneIntegrityCheckPassed;
    const char *sceneIntegrityStatusMessage;
} AVD_SceneManager;

bool avdSceneManagerInit(AVD_SceneManager *sceneManager, struct AVD_AppState *appState);
void avdSceneManagerDestroy(AVD_SceneManager *sceneManager, struct AVD_AppState *appState);
bool avdSceneManagerUpdate(AVD_SceneManager *sceneManager, struct AVD_AppState *appState);
bool avdSceneManagerRender(AVD_SceneManager *sceneManager, struct AVD_AppState *appState);
void avdSceneManagerPushInputEvent(AVD_SceneManager *sceneManager, struct AVD_AppState *appState, AVD_InputEvent *event);
bool avdSceneManagerCheckSceneIntegrity(AVD_SceneManager *sceneManager, AVD_SceneType type, struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneManagerSwitchToScene(AVD_SceneManager *sceneManager, AVD_SceneType type, struct AVD_AppState *appState);

#define AVD_FILE_INTEGRITY_CHECK(path) \
    if (!avdPathExists(path)) { \
        static char statusBuffer[256]; \
        snprintf(statusBuffer, sizeof(statusBuffer), "File integrity check failed: %s does not exist", path); \
        *statusMessage = statusBuffer; \
        AVD_CHECK_MSG(false, "File integrity check failed: %s does not exist", path); \
    }

#endif // AVD_SCENES_H