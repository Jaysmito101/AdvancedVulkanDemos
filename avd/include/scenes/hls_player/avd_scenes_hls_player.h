#ifndef AVD_SCENES_HLS_PLAYER_H
#define AVD_SCENES_HLS_PLAYER_H

#include "math/avd_math_base.h"
#include "pico/picoM3U8.h"
#include "pico/picoThreads.h"
#include "scenes/avd_scenes_base.h"
#include "vulkan/avd_vulkan_video.h"
#include <stdint.h>

#define AVD_SCENE_HLS_PLAYER_MAX_SOURCES                4
#define AVD_SCENE_HLS_PLAYER_NUM_SOURCE_WORKERS         4
#define AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DOWNLOAD_WORKERS 8
#define AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DEMUX_WORKERS    2
#define AVD_SCENE_HLS_PLAYER_MEDIA_BUFFER_CACHE_SIZE    16
#define AVD_SCENE_HLS_PLAYER_MEDIA_SEGMENTS_LOADED      16

typedef struct {
    char url[1024];
    bool active;
    AVD_Float refreshIntervalMs;
    AVD_Float lastRefreshed;
    AVD_UInt32 currentSegmentIndex;
    AVD_Float currentSegmentStartTime;
} AVD_SceneHLSPlayerSource;

typedef struct {
    AVD_UInt32 id;
    AVD_Float duration;
    AVD_UInt32 sourceIndex;
    AVD_Float refreshIntervalMs;
    AVD_H264Video* video;
} AVD_SceneHLSPlayerMediaSegment;

typedef struct {
    char url[1024];
    AVD_UInt32 sourcesHash;
    AVD_SceneHLSPlayerMediaSegment segment;
} AVD_SceneHLSPlayerSourceWorkerPayload;

typedef struct {
    char segmentUrl[1024];
    AVD_UInt32 sourcesHash;
    AVD_SceneHLSPlayerMediaSegment segment;
} AVD_SceneHLSPlayerMediaWorkerPayload;

typedef struct {
    char *data;
    AVD_Size dataSize;
    AVD_UInt32 sourcesHash;
    AVD_SceneHLSPlayerMediaSegment segment;
} AVD_SceneHLSPlayerDemuxWorkerPayload;

typedef struct {
    AVD_SceneHLSPlayerMediaSegment segment;
    AVD_UInt32 sourcesHash;
} AVD_SceneHLSPlayerMediaSegmentPayload;

typedef struct {
    char *data;
    AVD_Size dataSize;
    AVD_UInt32 key;
    AVD_UInt32 timestamp;
} AVD_SceneHLSPlayerMediaBufferCacheEntry;

typedef struct {
    AVD_SceneHLSPlayerMediaBufferCacheEntry entries[AVD_SCENE_HLS_PLAYER_MEDIA_BUFFER_CACHE_SIZE];
    picoThreadMutex mutex;
} AVD_SceneHLSPlayerMediaBufferCache;

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

    AVD_SceneHLSPlayerMediaBufferCache mediaBufferCache;

    AVD_SceneHLSPlayerMediaSegment loadedSegments[AVD_SCENE_HLS_PLAYER_MAX_SOURCES][AVD_SCENE_HLS_PLAYER_MEDIA_SEGMENTS_LOADED];

    bool isSupported;
    
    AVD_VulkanVideoDecoder vulkanVideo;

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
