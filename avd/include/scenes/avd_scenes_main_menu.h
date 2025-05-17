#ifndef AVD_SCENES_MAIN_MENU_H
#define AVD_SCENES_MAIN_MENU_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneMainMenu
{
    AVD_SceneType type;
    AVD_RenderableText title;

    int32_t loadingCount;
} AVD_SceneMainMenu;

bool avdSceneMainMenuInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneMainMenuRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneMainMenuUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneMainMenuDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneMainMenuLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char** statusMessage, float* progress);

bool avdSceneMainMenuCheckIntegrity(struct AVD_AppState *appState, const char** statusMessage);
bool avdSceneMainMenuRegisterApi(AVD_SceneAPI *api);



#endif // AVD_SCENES_MAIN_MENU_H