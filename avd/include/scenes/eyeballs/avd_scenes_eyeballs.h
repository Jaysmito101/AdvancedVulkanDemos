#ifndef AVD_SCENES_EYEBALLS_H
#define AVD_SCENES_EYEBALLS_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneEyeballs {
    AVD_SceneType type;

    AVD_RenderableText title;
    AVD_RenderableText info;

} AVD_SceneEyeballs;

bool avdSceneEyeballsInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneEyeballsRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneEyeballsUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneEyeballsDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneEyeballsLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneEyeballsInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneEyeballsCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneEyeballsRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_EYEBALLS_H