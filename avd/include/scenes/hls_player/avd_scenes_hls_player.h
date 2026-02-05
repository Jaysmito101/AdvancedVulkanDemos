#ifndef AVD_SCENES_HLS_PLAYER_H
#define AVD_SCENES_HLS_PLAYER_H

#include "audio/avd_audio.h"
#include "avd_scene_hls_player_segment_store.h"
#include "avd_scene_hls_worker_pool.h"
#include "core/avd_types.h"
#include "math/avd_vector_non_simd.h"
#include "scenes/avd_scenes_base.h"
#include "scenes/hls_player/avd_scene_hls_media_cache.h"
#include "scenes/hls_player/avd_scene_hls_url_pool.h"
#include "scenes/hls_player/avd_scene_hls_worker_pool.h"

#include "scenes/hls_player/avd_scene_hls_player_context.h"

#include "vulkan/avd_vulkan_image.h"
#include "vulkan/avd_vulkan_video.h"

#define AVD_SCENE_HLS_PLAYER_MAX_SOURCES 4
#define AVD_SCENE_HLS_PLAYER_SAVE_SEGMENTS_TO_DISK

typedef struct {
    char url[1024];
    bool active;
    AVD_Float refreshIntervalMs;
    AVD_Float lastRefreshed;
    AVD_Float videoStartTime;
    AVD_Size currentlyPlayingSegmentId;

    AVD_Bool firstBound;
    AVD_SceneHLSPlayerContext player;
} AVD_SceneHLSPlayerSource;

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

    AVD_SceneHLSPlayerSource sources[AVD_SCENE_HLS_PLAYER_MAX_SOURCES];
    AVD_UInt32 sourceCount;
    AVD_UInt32 sourcesHash;

    AVD_HLSURLPool urlPool;
    AVD_HLSMediaCache mediaCache;
    AVD_HLSWorkerPool workerPool;

    AVD_Vector3 cameraPosition;
    AVD_Vector3 cameraDirection;
    AVD_Float cameraYaw;
    AVD_Float cameraPitch;

    bool isSupported;
    AVD_Vulkan *vulkan;
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
