#ifndef AVD_SCENES_HLS_PLAYER_H
#define AVD_SCENES_HLS_PLAYER_H

#include "math/avd_math_base.h"
#include "pico/picoM3U8.h"
#include "pico/picoThreads.h"
#include "scenes/avd_scenes_base.h"

#define AVD_SCENE_HLS_PLAYER_MAX_SOURCES                8
#define AVD_SCENE_HLS_PLAYER_NUM_SOURCE_WORKERS         2
#define AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DOWNLOAD_WORKERS 4
#define AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DEMUX_WORKERS    2

typedef struct {
    char url[1024];
    bool active;
    AVD_Float refreshIntervalMs;
    AVD_Float lastRefreshed;
} AVD_SceneHLSPlayerSource;

typedef struct {
    char url[1024];
    AVD_UInt32 sourcesHash;
    AVD_UInt32 sourceIndex;
} AVD_SceneHLSPlayerSourceWorkerPayload;

typedef struct {
    char segmentUrl[1024];
    AVD_UInt32 sourcesHash;
    AVD_UInt32 sourceIndex;
    AVD_UInt32 segmentIndex;
    AVD_Float refreshIntervalMs;
} AVD_SceneHLSPlayerMediaWorkerPayload;

typedef struct {
    char* data;
    AVD_Size dataSize;
    AVD_UInt32 sourcesHash;
    AVD_UInt32 sourceIndex;
    AVD_UInt32 segmentIndex;
    AVD_Float refreshIntervalMs;
} AVD_SceneHLSPlayerDemuxWorkerPayload;

typedef struct {
    // TODO: some thing that holds the demuxed media data
    AVD_UInt32 sourcesHash;
    AVD_UInt32 sourceIndex;
    AVD_UInt32 segmentIndex;
    AVD_Float refreshIntervalMs;
} AVD_SceneHLSPlayerMediaSegmentPayload;

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

    picoThread sourceDownloadWorker[AVD_SCENE_HLS_PLAYER_NUM_SOURCE_WORKERS];
    bool sourceDownloadWorkerRunning;

    picoThread mediaDownloadWorker[AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DOWNLOAD_WORKERS];
    bool mediaDownloadWorkerRunning;

    picoThread mediaDemuxWorker[AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DEMUX_WORKERS];
    bool mediaDemuxWorkerRunning;

    picoThreadChannel sourceDownloadChannel; // main thread -> source download worker
    picoThreadChannel mediaDownloadChannel;  // source worker -> media download worker
    picoThreadChannel mediaDemuxChannel;     // medua download worker -> media demux worker
    picoThreadChannel mediaReadyChannel;     // media demux worker -> main thread

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
