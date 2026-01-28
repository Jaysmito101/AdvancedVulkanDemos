#ifndef AVD_SCENES_HLS_PLAYER_H
#define AVD_SCENES_HLS_PLAYER_H

#include "core/avd_types.h"
#include "scenes/avd_scenes_base.h"
#include "scenes/hls_player/avd_scene_hls_media_cache.h"
#include "scenes/hls_player/avd_scene_hls_segment_store.h"
#include "scenes/hls_player/avd_scene_hls_url_pool.h"
#include "scenes/hls_player/avd_scene_hls_worker_pool.h"

#include "vulkan/avd_vulkan_video.h"

#define AVD_SCENE_HLS_PLAYER_MAX_SOURCES 4
#define AVD_SCENE_HLS_PLAYER_SAVE_SEGMENTS_TO_DISK

typedef struct {
    char url[1024];
    bool active;
    AVD_Float refreshIntervalMs;
    AVD_Float lastRefreshed;
    AVD_Float videoStartTime;

    AVD_UInt32 currentSegmentIndex;
    AVD_Float currentSegmentStartTime;
    AVD_Float currentsegmentDuration;
    AVD_Size currentSegmentFrameCount;

    AVD_UInt32 lastLoadedSegmentIndex;
    AVD_Bool decoderReady;
    AVD_VulkanVideoDecoder videoDecoder;
    AVD_Bool decodedThisFrame;
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
    AVD_HLSSegmentStore segmentStore;
    AVD_HLSWorkerPool workerPool;

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
