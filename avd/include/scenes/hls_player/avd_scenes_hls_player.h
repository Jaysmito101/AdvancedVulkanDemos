#ifndef AVD_SCENES_HLS_PLAYER_H
#define AVD_SCENES_HLS_PLAYER_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneHLSPlayer {
    AVD_SceneType type;

    AVD_Float sceneWidth;
    AVD_Float sceneHeight;
    AVD_Matrix4x4 viewModelMatrix;
    AVD_Matrix4x4 projectionMatrix;

    AVD_RenderableText title;
    AVD_RenderableText info;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    bool isSupported;

} AVD_SceneHLSPlayer;

bool avdSceneHLSPlayerInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneHLSPlayerRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneHLSPlayerUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneHLSPlayerDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneHLSPlayerLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneHLSPlayerInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneHLSPlayerCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneHLSPlayerRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_HLS_PLAYER_H