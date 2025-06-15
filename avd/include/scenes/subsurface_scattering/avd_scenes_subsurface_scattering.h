
#ifndef AVD_SCENES_SUBSURFACE_SCATTERING_H
#define AVD_SCENES_SUBSURFACE_SCATTERING_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneSubsurfaceScattering {
    AVD_SceneType type;
    
    AVD_RenderableText title;
} AVD_SceneSubsurfaceScattering;

bool avdSceneSubsurfaceScatteringInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneSubsurfaceScatteringRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneSubsurfaceScatteringUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneSubsurfaceScatteringDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneSubsurfaceScatteringLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneSubsurfaceScatteringInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneSubsurfaceScatteringCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneSubsurfaceScatteringRegisterApi(AVD_SceneAPI *api);



#endif // AVD_SCENES_SUBSURFACE_SCATTERING_H