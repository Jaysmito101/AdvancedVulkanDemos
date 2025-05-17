#ifndef AVD_SCENES_H
#define AVD_SCENES_H

#include "scenes/avd_scenes_base.h"
#include "scenes/avd_scenes_main_menu.h"

typedef union AVD_Scene {
    AVD_SceneType type;
    AVD_SceneMainMenu mainMenu;    
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
bool avdSceneManagerCheckSceneIntegrity(AVD_SceneManager *sceneManager, AVD_SceneType type, struct AVD_AppState *appState, const char** statusMessage);
bool avdSceneManagerSwitchToScene(AVD_SceneManager *sceneManager, AVD_SceneType type, struct AVD_AppState *appState);


#endif // AVD_SCENES_H