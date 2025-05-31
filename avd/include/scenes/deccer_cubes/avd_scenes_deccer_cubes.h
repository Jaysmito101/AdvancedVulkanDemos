
#ifndef AVD_SCENES_DECCER_CUBES_H
#define AVD_SCENES_DECCER_CUBES_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneDeccerCubes {
    AVD_SceneType type;
    
    AVD_RenderableText title;
} AVD_SceneDeccerCubes;

bool avdSceneDeccerCubesInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneDeccerCubesRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneDeccerCubesUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneDeccerCubesDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneDeccerCubesLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneDeccerCubesInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneDeccerCubesCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneDeccerCubesRegisterApi(AVD_SceneAPI *api);



#endif // AVD_SCENES_DECCER_CUBES_H