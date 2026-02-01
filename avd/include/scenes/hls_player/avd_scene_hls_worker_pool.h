#ifndef AVD_HLS_WORKER_POOL_H
#define AVD_HLS_WORKER_POOL_H

#include "core/avd_types.h"
#include "pico/picoThreads.h"
#include "scenes/hls_player/avd_scene_hls_media_cache.h"
#include "scenes/hls_player/avd_scene_hls_url_pool.h"
#include "scenes/hls_player/avd_scene_hls_player_segment_store.h"
#include <stdint.h>

#define AVD_HLS_WORKER_NUM_SOURCE_WORKERS         4
#define AVD_HLS_WORKER_NUM_MEDIA_DOWNLOAD_WORKERS 8
#define AVD_HLS_WORKER_NUM_MEDIA_DEMUX_WORKERS    2

#ifdef AVD_HLS_WORKER_POOL_LOG
#define AVD_HLS_WORKER_POOL_LOG(...) AVD_LOG_DEBUG("[HLS WORKER] " __VA_ARGS__)
#else 
#define AVD_HLS_WORKER_POOL_LOG(...) do { (void)0; } while(0)
#endif

struct AVD_SceneHLSPlayer;

typedef struct {
    AVD_UInt32 sourceIndex;
    AVD_UInt32 sourcesHash;
    AVD_UInt32 sourceUrlHash;
} AVD_HLSSourceTaskPayload;

typedef struct {
    AVD_UInt32 segmentId;
    AVD_UInt32 sourceIndex;
    AVD_Float duration;
    AVD_Float refreshIntervalMs;
    AVD_UInt32 urlHash;
    AVD_UInt32 sourcesHash;
} AVD_HLSMediaTaskPayload;

typedef struct {
    AVD_UInt32 segmentId;
    AVD_UInt32 sourceIndex;
    AVD_Float duration;
    char *data;
    AVD_Size dataSize;
    AVD_UInt32 sourcesHash;
} AVD_HLSDemuxTaskPayload;

typedef struct {
    AVD_HLSSegmentAVData avData;
    AVD_UInt32 sourcesHash;
} AVD_HLSReadyPayload;

typedef struct {
    picoThread sourceDownloadWorkers[AVD_HLS_WORKER_NUM_SOURCE_WORKERS];
    picoThread mediaDownloadWorkers[AVD_HLS_WORKER_NUM_MEDIA_DOWNLOAD_WORKERS];
    picoThread mediaDemuxWorkers[AVD_HLS_WORKER_NUM_MEDIA_DEMUX_WORKERS];

    bool sourceDownloadRunning;
    bool mediaDownloadRunning;
    bool mediaDemuxRunning;

    picoThreadChannel sourceDownloadChannel;
    picoThreadChannel mediaDownloadChannel;
    picoThreadChannel mediaDemuxChannel;
    picoThreadChannel mediaReadyChannel;

    struct AVD_SceneHLSPlayer *parentScene;
    AVD_HLSURLPool *urlPool;
    AVD_HLSMediaCache *mediaCache;
} AVD_HLSWorkerPool;

bool avdHLSWorkerPoolInit(AVD_HLSWorkerPool *pool, AVD_HLSURLPool *urlPool, AVD_HLSMediaCache *mediaCache, struct AVD_SceneHLSPlayer *parentScene);
void avdHLSWorkerPoolDestroy(AVD_HLSWorkerPool *pool);
void avdHLSWorkerPoolFlush(AVD_HLSWorkerPool *pool);

bool avdHLSWorkerPoolSendSourceTask(AVD_HLSWorkerPool *pool, AVD_UInt32 sourceIndex, AVD_UInt32 sourcesHash, const char *sourceUrl);
bool avdHLSWorkerPoolReceiveReadySegment(AVD_HLSWorkerPool *pool, AVD_HLSReadyPayload *outPayload);


#endif // AVD_HLS_WORKER_POOL_H
