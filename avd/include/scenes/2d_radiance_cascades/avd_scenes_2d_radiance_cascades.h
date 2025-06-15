
#ifndef AVD_SCENES_2D_RADIANCE_CASCADES_H
#define AVD_SCENES_2D_RADIANCE_CASCADES_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_Scene2DRadianceCascades {
    AVD_SceneType type;

    AVD_RenderableText title;
} AVD_Scene2DRadianceCascades;

bool avdScene2DRadianceCascadesInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdScene2DRadianceCascadesRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdScene2DRadianceCascadesUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdScene2DRadianceCascadesDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdScene2DRadianceCascadesLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdScene2DRadianceCascadesInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdScene2DRadianceCascadesCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdScene2DRadianceCascadesRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_2D_RADIANCE_CASCADES_H