#ifndef AVD_SCENES_EROSION_SIMULATION_H
#define AVD_SCENES_EROSION_SIMULATION_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneErosionSimulation {
    AVD_SceneType type;

    AVD_RenderableText title;
    AVD_RenderableText status;
} AVD_SceneErosionSimulation;

bool avdSceneErosionSimulationInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneErosionSimulationRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneErosionSimulationUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneErosionSimulationDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneErosionSimulationLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneErosionSimulationInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneErosionSimulationCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneErosionSimulationRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_EROSION_SIMULATION_H
