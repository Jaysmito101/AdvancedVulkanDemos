#include "scenes/hls_player/avd_scenes_hls_player.h"
#include "avd_application.h"
#include "core/avd_base.h"
#include "core/avd_utils.h"
#include "font/avd_font.h"
#include "math/avd_math_base.h"
#include "pico/picoM3U8.h"
#include "pico/picoThreads.h"
#include "scenes/avd_scenes.h"
#include <objidl.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t activeSources; // each source has got 4 bits (4 * 8 = 32)
    int32_t indexCount;
    int32_t pad0;
    int32_t pad1;
} AVD_HLSPlayerPushConstants;

static AVD_SceneHLSPlayer *__avdSceneGetTypePtr(union AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_HLS_PLAYER);
    return &scene->hlsPlayer;
}

static void __avdSceneHLSPlayerFreeMediaSegment(void *segmentRaw, void *context)
{
    (void)context;

    AVD_SceneHLSPlayerMediaSegmentPayload *segment = (AVD_SceneHLSPlayerMediaSegmentPayload *)segmentRaw;
    AVD_ASSERT(segment != NULL);
    // TODO: To be implemented when we have actual demuxed data
}

static void __avdSceneHLSPlayerFreeDemuxWorkerPayload(void *segmentRaw, void *context)
{
    (void)context;

    AVD_SceneHLSPlayerDemuxWorkerPayload *segment = (AVD_SceneHLSPlayerDemuxWorkerPayload *)segmentRaw;
    AVD_ASSERT(segment != NULL);

    if (segment->data) {
        free(segment->data);
        segment->data     = NULL;
        segment->dataSize = 0;
    }
}

static AVD_UInt32 __avdSceneHLSPlayerUpdateActiveSources(AVD_SceneHLSPlayer *scene)
{
    AVD_UInt32 result = 0;
    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_MAX_SOURCES; i++) {
        result |= (AVD_UInt32)(scene->sources[i].active ? 1 : 0) << i * 4;
    }
    return result;
}

static bool __avdSceneHLSPlayerInitializeMediaBufferCache(AVD_SceneHLSPlayer *scene)
{
    AVD_ASSERT(scene != NULL);

    picoThreadMutex mutex = picoThreadMutexCreate();
    AVD_CHECK_MSG(mutex != NULL, "Failed to create media buffer cache mutex");
    scene->mediaBufferCache.mutex = mutex;
    memset(scene->mediaBufferCache.entries, 0, sizeof(AVD_SceneHLSPlayerMediaBufferCacheEntry) * AVD_SCENE_HLS_PLAYER_MEDIA_BUFFER_CACHE_SIZE);
    return true;
}

static void __avdSceneHLSPlayerShutdownMediaBufferCache(AVD_SceneHLSPlayer *scene)
{
    AVD_ASSERT(scene != NULL);

    picoThreadMutex mutex = scene->mediaBufferCache.mutex;
    if (mutex) {
        picoThreadMutexLock(mutex, PICO_THREAD_INFINITE);
        for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_MEDIA_BUFFER_CACHE_SIZE; i++) {
            AVD_SceneHLSPlayerMediaBufferCacheEntry *entry = &scene->mediaBufferCache.entries[i];
            if (entry->key && entry->data) {
                free(entry->data);
                entry->data     = NULL;
                entry->dataSize = 0;
            }
        }
        picoThreadMutexUnlock(mutex);
        picoThreadMutexDestroy(mutex);
        scene->mediaBufferCache.mutex = NULL;
    }
}

static bool __avdSceneHLSPlayerQueryMediaBufferCache(AVD_SceneHLSPlayer *scene, AVD_UInt32 key, char **data, AVD_Size *dataSize)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(data != NULL);
    AVD_ASSERT(dataSize != NULL);

    picoThreadMutexLock(scene->mediaBufferCache.mutex, PICO_THREAD_INFINITE);
    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_MEDIA_BUFFER_CACHE_SIZE; i++) {
        AVD_SceneHLSPlayerMediaBufferCacheEntry *entry = &scene->mediaBufferCache.entries[i];
        if (entry->key == key) {
            *data = malloc(entry->dataSize);
            AVD_CHECK_MSG(*data != NULL, "Failed to allocate memory for media buffer cache entry data");
            memcpy(*data, entry->data, entry->dataSize);
            *dataSize        = entry->dataSize;
            entry->timestamp = time(NULL);
            picoThreadMutexUnlock(scene->mediaBufferCache.mutex);
            return true;
        }
    }
    picoThreadMutexUnlock(scene->mediaBufferCache.mutex);
    return false;
}

static bool __avdSceneHLSPlayerInsertMediaBufferCache(AVD_SceneHLSPlayer *scene, AVD_UInt32 key, const char *data, AVD_Size dataSize)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(data != NULL);
    AVD_ASSERT(dataSize > 0);

    picoThreadMutexLock(scene->mediaBufferCache.mutex, PICO_THREAD_INFINITE);
    AVD_Size lruIndex       = 0;
    AVD_UInt32 lruTimestamp = UINT32_MAX;
    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_MEDIA_BUFFER_CACHE_SIZE; i++) {
        AVD_SceneHLSPlayerMediaBufferCacheEntry *entry = &scene->mediaBufferCache.entries[i];
        if (entry->key == 0) {
            entry->data = malloc(dataSize);
            AVD_CHECK_MSG(entry->data != NULL, "Failed to allocate memory for media buffer cache entry data");
            memcpy(entry->data, data, dataSize);
            entry->dataSize  = dataSize;
            entry->key       = key;
            entry->timestamp = time(NULL);
            picoThreadMutexUnlock(scene->mediaBufferCache.mutex);
            return true;
        }
        if (entry->timestamp < lruTimestamp) {
            lruTimestamp = entry->timestamp;
            lruIndex     = i;
        }
    }
    // no empty slot found, replace the least recently used slot
    AVD_SceneHLSPlayerMediaBufferCacheEntry *entry = &scene->mediaBufferCache.entries[lruIndex];
    free(entry->data);
    entry->data = malloc(dataSize);
    AVD_CHECK_MSG(entry->data != NULL, "Failed to allocate memory for media buffer cache entry data");
    memcpy(entry->data, data, dataSize);
    entry->dataSize  = dataSize;
    entry->key       = key;
    entry->timestamp = time(NULL);
    picoThreadMutexUnlock(scene->mediaBufferCache.mutex);
    return true;
}

static bool __avdSceneHLSPlayerLoadSourcesFromPath(AVD_SceneHLSPlayer *scene, const char *path)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(path != NULL);

    // path should have a text file with sources listed line by line
    char *fileData  = NULL;
    size_t fileSize = 0;
    if (!avdReadBinaryFile(path, (void **)&fileData, &fileSize)) {
        AVD_LOG_ERROR("Failed to read HLS sources file: %s", path);
        return false;
    }

    const char *lineStart = fileData;

    AVD_Size sourceIndex = 0;
    while (*lineStart && sourceIndex < AVD_SCENE_HLS_PLAYER_MAX_SOURCES) {
        const char *lineEnd = strchr(lineStart, '\n');
        if (!lineEnd)
            lineEnd = strchr(lineStart, '\0');

        size_t lineLength = lineEnd - lineStart;
        if (lineLength > 0) {
            if (lineLength >= sizeof(scene->sources[sourceIndex].url)) {
                AVD_LOG_WARN("HLS source URL too long, truncating: %.*s", (int)lineLength, lineStart);
                lineLength = sizeof(scene->sources[sourceIndex].url) - 1;
            }
            if (!avdIsStringAURL(lineStart)) {
                AVD_LOG_WARN("Invalid URL format for HLS source, skipping: %.*s", (int)lineLength, lineStart);
                lineStart = (*lineEnd == '\0') ? lineEnd : lineEnd + 1;
                continue;
            }
            AVD_LOG_INFO("Loaded HLS source: %.*s", (int)lineLength, lineStart);
            strncpy(scene->sources[sourceIndex].url, lineStart, lineLength);
            scene->sources[sourceIndex].url[lineLength]         = '\0';
            scene->sources[sourceIndex].active                  = true;
            scene->sources[sourceIndex].refreshIntervalMs       = 10000000.0f; // by default have this very high
            scene->sources[sourceIndex].lastRefreshed           = -1.0 * scene->sources[sourceIndex].refreshIntervalMs;
            scene->sources[sourceIndex].currentSegmentIndex     = 0;
            scene->sources[sourceIndex].currentSegmentStartTime = 0.0f;

            memset(&scene->loadedSegments[sourceIndex], 0, sizeof(AVD_SceneHLSPlayerMediaSegment) * AVD_SCENE_HLS_PLAYER_MEDIA_SEGMENTS_LOADED);

            sourceIndex++;
        }

        lineStart = (*lineEnd == '\0') ? lineEnd : lineEnd + 1;
    }
    scene->sourceCount = (AVD_UInt32)sourceIndex;
    scene->sourcesHash = avdHashBuffer(scene->sources, sizeof(AVD_SceneHLSPlayerSource) * scene->sourceCount);

    AVD_LOG_INFO("Total HLS sources loaded: %d [hash: 0x%08X]", scene->sourceCount, scene->sourcesHash);

    free(fileData);
    return true;
}

static bool __avdSceneHLSPlayerFreeSources(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    (void)appState;
    for (AVD_Size i = 0; i < scene->sourceCount; i++) {
        if (scene->sources[i].active) {
            scene->sources[i].active = false;
        }
    }
    scene->sourceCount = 0;
    return true;
}

static bool __avdSceneHLSPlayerUpdateSources(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    AVD_Float time = (AVD_Float)appState->framerate.currentTime;
    for (AVD_Size i = 0; i < scene->sourceCount; i++) {
        AVD_SceneHLSPlayerSource *source = &scene->sources[i];
        if (!source->active) {
            continue;
        }

        if (source->lastRefreshed + source->refreshIntervalMs < time) {
            static AVD_SceneHLSPlayerSourceWorkerPayload payload = {0};
            memcpy(payload.url, source->url, sizeof(payload.url));
            payload.sourcesHash         = scene->sourcesHash;
            payload.segment.sourceIndex = (AVD_UInt32)i;
            AVD_CHECK_MSG(picoThreadChannelSend(scene->sourceDownloadChannel, &payload), "Failed to send source download payload to worker thread");
            source->lastRefreshed = time;
            continue;
        }


    }

    return true;
}

static bool __avdSceneHLSPlayerRecieveReadySegments(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    AVD_SceneHLSPlayerMediaSegmentPayload payload = {0};
    while (picoThreadChannelTryReceive(scene->mediaReadyChannel, &payload)) {
        AVD_LOG_VERBOSE("HLS Main Thread received ready segment for source index %u, segment index %u", payload.segment.sourceIndex, payload.segment.id);
        if (payload.sourcesHash != scene->sourcesHash) {
            AVD_LOG_WARN("HLS Main Thread received segment for outdated sources hash 0x%08X (current: 0x%08X), discarding", payload.sourcesHash, scene->sourcesHash);
            __avdSceneHLSPlayerFreeMediaSegment(&payload, NULL);
            continue;
        }

        scene->sources[payload.segment.sourceIndex].refreshIntervalMs = payload.segment.refreshIntervalMs;

        // for an empty slot, insert the segment
        bool segmentInserted = false;
        for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_MEDIA_SEGMENTS_LOADED; i++) {
            if (scene->loadedSegments[payload.segment.sourceIndex][i].id == 0) {
                scene->loadedSegments[payload.segment.sourceIndex][i] = payload.segment;
                segmentInserted                               = true;
                AVD_LOG_INFO("HLS Main Thread inserted loaded segment %u for source index %u into slot %u", payload.segment.id, payload.segment.sourceIndex, (AVD_UInt32)i);
                break;
            }
        }
        if (!segmentInserted) {
            AVD_LOG_WARN("HLS Main Thread could not insert loaded segment %u for source index %u: no empty slots available", payload.segment.id, payload.segment.sourceIndex);
        }
        __avdSceneHLSPlayerFreeMediaSegment(&payload, NULL);
    }

    return true;
}

static void __avdSceneHLSSourceDownloadWorker(void *arg)
{
    AVD_SceneHLSPlayer *scene          = (AVD_SceneHLSPlayer *)arg;
    scene->sourceDownloadWorkerRunning = true;
    AVD_LOG_INFO("HLS Data Worker thread started with thread ID: %llu.", picoThreadGetCurrentId());
    AVD_SceneHLSPlayerSourceWorkerPayload payload     = {0};
    picoM3U8Playlist sourcePlaylist                   = NULL;
    AVD_SceneHLSPlayerMediaWorkerPayload mediaPayload = {0};
    while (scene->sourceDownloadWorkerRunning) {
        if (picoThreadChannelReceive(scene->sourceDownloadChannel, &payload, 1000)) {
            char *data = NULL;
            if (!avdCurlFetchStringContent(payload.url, &data, NULL)) {
                AVD_LOG_ERROR("Failed to fetch HLS playlist for source: %s", payload.url);
                continue;
            }
            if (picoM3U8PlaylistParse(data, (uint32_t)strlen(data), &sourcePlaylist) != PICO_M3U8_RESULT_SUCCESS) {
                AVD_LOG_ERROR("Failed to parse HLS playlist for source: %s", payload.url);
                free(data);
                continue;
            }

            if (sourcePlaylist->type != PICO_M3U8_PLAYLIST_TYPE_MEDIA) {
                AVD_LOG_ERROR("Master playlists are not supported for yet!: %s", payload.url);
                picoM3U8PlaylistDestroy(sourcePlaylist);
                free(data);
            }

            mediaPayload.sourcesHash               = payload.sourcesHash;
            mediaPayload.segment                   = payload.segment;
            mediaPayload.segment.refreshIntervalMs = sourcePlaylist->media.targetDuration;

            for (AVD_UInt32 segmentIndex = 0; segmentIndex < sourcePlaylist->media.mediaSegmentCount; segmentIndex++) {
                picoM3U8MediaSegment segment = &sourcePlaylist->media.mediaSegments[segmentIndex];
                avdResolveRelativeURL(mediaPayload.segmentUrl, sizeof(mediaPayload.segmentUrl), payload.url, segment->uri);
                mediaPayload.segment.id       = segmentIndex + sourcePlaylist->media.mediaSequence;
                mediaPayload.segment.duration = segment->duration;
                if (!picoThreadChannelSend(scene->mediaDownloadChannel, &mediaPayload)) {
                    AVD_LOG_ERROR("Failed to send media download payload to worker thread");
                }
            }

            picoM3U8PlaylistDestroy(sourcePlaylist);
            free(data);
        }
    }
    AVD_LOG_INFO("HLS Data Worker thread stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

static void __avdSceneHLSMediaDownloadWoker(void *args)
{
    AVD_SceneHLSPlayer *scene         = (AVD_SceneHLSPlayer *)args;
    scene->mediaDownloadWorkerRunning = true;
    AVD_LOG_INFO("HLS Media Download Worker thread started with thread ID: %llu.", picoThreadGetCurrentId());

    AVD_SceneHLSPlayerDemuxWorkerPayload demuxPayload = {0};
    AVD_SceneHLSPlayerMediaWorkerPayload payload      = {0};
    while (scene->mediaDownloadWorkerRunning) {
        if (picoThreadChannelReceive(scene->mediaDownloadChannel, &payload, 1000)) {
            demuxPayload.segment     = payload.segment;
            demuxPayload.sourcesHash = payload.sourcesHash;

            if (__avdSceneHLSPlayerQueryMediaBufferCache(scene, avdHashString(payload.segmentUrl), &demuxPayload.data, &demuxPayload.dataSize)) {
                AVD_LOG_INFO("HLS Media Download Worker cache hit for source index %u, segment index %u, url: %s", payload.segment.sourceIndex, payload.segment.id, payload.segmentUrl);
                if (!picoThreadChannelSend(scene->mediaDemuxChannel, &demuxPayload)) {
                    AVD_LOG_ERROR("Failed to send media demux payload to worker thread");
                }
                continue;
            }

            if (!avdCurlDownloadToMemory(payload.segmentUrl, (void **)&demuxPayload.data, &demuxPayload.dataSize)) {
                AVD_LOG_ERROR("Failed to download media segment from url: %s", payload.segmentUrl);
                continue;
            }

            if (!__avdSceneHLSPlayerInsertMediaBufferCache(scene, avdHashString(payload.segmentUrl), demuxPayload.data, demuxPayload.dataSize)) {
                AVD_LOG_WARN("Failed to insert downloaded media segment into cache for source index %u, segment index %u, url: %s", payload.segment.sourceIndex, payload.segment.id, payload.segmentUrl);
            } else {
                AVD_LOG_INFO("HLS Media Download Worker cached segment for source index %u, segment index %u, url: %s", payload.segment.sourceIndex, payload.segment.id, payload.segmentUrl);
            }

            if (!picoThreadChannelSend(scene->mediaDemuxChannel, &demuxPayload)) {
                AVD_LOG_ERROR("Failed to send media demux payload to worker thread");
            }
        }
    }
    AVD_LOG_INFO("HLS Media Download Worker thread stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

static void __avdSceneHLSMediaDemuxWorker(void *args)
{
    AVD_SceneHLSPlayer *scene      = (AVD_SceneHLSPlayer *)args;
    scene->mediaDemuxWorkerRunning = true;
    AVD_LOG_INFO("HLS Media Demux Worker thread started with thread ID: %llu.", picoThreadGetCurrentId());
    AVD_SceneHLSPlayerDemuxWorkerPayload payload         = {0};
    AVD_SceneHLSPlayerMediaSegmentPayload segmentPayload = {0};
    while (scene->mediaDemuxWorkerRunning) {
        if (picoThreadChannelReceive(scene->mediaDemuxChannel, &payload, 1000)) {
            free(payload.data); // TODO: actually demux the data here

            segmentPayload.sourcesHash       = payload.sourcesHash;
            segmentPayload.segment           = payload.segment;
            if (!picoThreadChannelSend(scene->mediaReadyChannel, &segmentPayload)) {
                AVD_LOG_ERROR("Failed to send media ready payload to main thread");
            }
        }
    }
    AVD_LOG_INFO("HLS Media Demux Worker thread stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

static bool __avdSceneHLSPlayerPrepWorkers(AVD_SceneHLSPlayer *scene)
{
    AVD_ASSERT(scene != NULL);

    // Create the thread channels to communicate between threads
    scene->sourceDownloadChannel = picoThreadChannelCreateUnbounded(sizeof(AVD_SceneHLSPlayerSourceWorkerPayload));
    AVD_CHECK_MSG(scene->sourceDownloadChannel != NULL, "Failed to create HLS source download channel");

    scene->mediaDownloadChannel = picoThreadChannelCreateUnbounded(sizeof(AVD_SceneHLSPlayerMediaWorkerPayload));
    AVD_CHECK_MSG(scene->mediaDownloadChannel != NULL, "Failed to create HLS media download channel");

    scene->mediaDemuxChannel = picoThreadChannelCreateUnbounded(sizeof(AVD_SceneHLSPlayerDemuxWorkerPayload));
    AVD_CHECK_MSG(scene->mediaDemuxChannel != NULL, "Failed to create HLS media demux channel");
    picoThreadChannelSetItemDestructor(scene->mediaDemuxChannel, __avdSceneHLSPlayerFreeDemuxWorkerPayload, NULL);

    scene->mediaReadyChannel = picoThreadChannelCreateUnbounded(sizeof(AVD_SceneHLSPlayerMediaSegmentPayload));
    AVD_CHECK_MSG(scene->mediaReadyChannel != NULL, "Failed to create HLS media ready channel");
    picoThreadChannelSetItemDestructor(scene->mediaReadyChannel, __avdSceneHLSPlayerFreeMediaSegment, NULL);

    // Spin up the threads
    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_SOURCE_WORKERS; i++) {
        scene->sourceDownloadWorker[i] = picoThreadCreate(__avdSceneHLSSourceDownloadWorker, scene);
        AVD_CHECK_MSG(scene->sourceDownloadWorker[i] != NULL, "Failed to create HLS data worker thread");
    }

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DOWNLOAD_WORKERS; i++) {
        scene->mediaDownloadWorker[i] = picoThreadCreate(__avdSceneHLSMediaDownloadWoker, scene);
        AVD_CHECK_MSG(scene->mediaDownloadWorker[i] != NULL, "Failed to create HLS data worker thread");
    }

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DEMUX_WORKERS; i++) {
        scene->mediaDemuxWorker[i] = picoThreadCreate(__avdSceneHLSMediaDemuxWorker, scene);
        AVD_CHECK_MSG(scene->mediaDemuxWorker[i] != NULL, "Failed to create HLS data worker thread");
    }

    return true;
}

static void __avdSceneHLSPlayerWindDownWorkers(AVD_SceneHLSPlayer *scene)
{
    AVD_ASSERT(scene != NULL);

    scene->sourceDownloadWorkerRunning = false;
    scene->mediaDownloadWorkerRunning  = false;
    scene->mediaDemuxWorkerRunning     = false;

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_SOURCE_WORKERS; i++) {
        picoThreadDestroy(scene->sourceDownloadWorker[i]);
    }

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DOWNLOAD_WORKERS; i++) {
        picoThreadDestroy(scene->mediaDownloadWorker[i]);
    }

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DEMUX_WORKERS; i++) {
        picoThreadDestroy(scene->mediaDemuxWorker[i]);
    }

    picoThreadChannelDestroy(scene->sourceDownloadChannel);
    picoThreadChannelDestroy(scene->mediaDownloadChannel);
    picoThreadChannelDestroy(scene->mediaDemuxChannel);
    picoThreadChannelDestroy(scene->mediaReadyChannel);
}

static bool avdSceneHLSPlayerCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    if (!appState->vulkan.supportedFeatures.videoDecode) {
        *statusMessage = "HLS Player scene is not supported on this GPU (video decode not supported).";
        return false;
    }

    if (!avdCurlIsSupported()) {
        *statusMessage = "HLS Player scene requires curl cli to be installed and available in PATH.";
        return false;
    }

    if (!avdPathExists("assets/scene_hls_player/sources.txt")) {
        AVD_LOG_WARN("HLS Player default sources file not found: assets/scene_hls_player/sources.txt");
    }

    return true;
}

bool avdSceneHLSPlayerRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneHLSPlayerCheckIntegrity;
    api->init           = avdSceneHLSPlayerInit;
    api->render         = avdSceneHLSPlayerRender;
    api->update         = avdSceneHLSPlayerUpdate;
    api->destroy        = avdSceneHLSPlayerDestroy;
    api->load           = avdSceneHLSPlayerLoad;
    api->inputEvent     = avdSceneHLSPlayerInputEvent;

    api->displayName = "HLS Player";
    api->id          = "Bloom";

    return true;
}

bool avdSceneHLSPlayerInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    memset(hlsPlayer->sources, 0, sizeof(hlsPlayer->sources));

    hlsPlayer->isSupported = appState->vulkan.supportedFeatures.videoDecode;

    AVD_CHECK(avdRenderableTextCreate(
        &hlsPlayer->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        hlsPlayer->isSupported ? "HLS Player" : "HLS Player (Not Supported on this GPU)",
        64.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &hlsPlayer->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Copy a url to a HLS stream and click the window and press P.",
        24.0f));

    hlsPlayer->sceneWidth  = (float)appState->renderer.sceneFramebuffer.width;
    hlsPlayer->sceneHeight = (float)appState->renderer.sceneFramebuffer.height;

    if (!hlsPlayer->isSupported) {
        return true;
    }

    AVD_VulkanPipelineCreationInfo pipelineCreationInfo = {0};
    avdPipelineUtilsPipelineCreationInfoInit(&pipelineCreationInfo);
    pipelineCreationInfo.enableDepthTest = true;
    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &hlsPlayer->pipelineLayout,
        &hlsPlayer->pipeline,
        appState->vulkan.device,
        &appState->vulkan.bindlessDescriptorSetLayout,
        1,
        sizeof(AVD_HLSPlayerPushConstants),
        appState->renderer.sceneFramebuffer.renderPass,
        (AVD_UInt32)appState->renderer.sceneFramebuffer.colorAttachments.count,
        "HLSPlayerVert",
        "HLSPlayerFrag",
        NULL,
        &pipelineCreationInfo));

    if (avdPathExists("assets/scene_hls_player/sources.txt")) {
        AVD_CHECK(__avdSceneHLSPlayerLoadSourcesFromPath(hlsPlayer, "assets/scene_hls_player/sources.txt"));
        AVD_LOG_INFO("Loaded HLS sources from sources.txt");
    }

    AVD_CHECK(__avdSceneHLSPlayerInitializeMediaBufferCache(hlsPlayer));
    AVD_CHECK(__avdSceneHLSPlayerPrepWorkers(hlsPlayer));

    return true;
}

void avdSceneHLSPlayerDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    __avdSceneHLSPlayerWindDownWorkers(hlsPlayer);
    __avdSceneHLSPlayerShutdownMediaBufferCache(hlsPlayer);
    __avdSceneHLSPlayerFreeSources(appState, hlsPlayer);

    avdRenderableTextDestroy(&hlsPlayer->title, &appState->vulkan);
    avdRenderableTextDestroy(&hlsPlayer->info, &appState->vulkan);

    if (!hlsPlayer->isSupported) {
        return;
    }

    vkDestroyPipeline(appState->vulkan.device, hlsPlayer->pipeline, NULL);
    vkDestroyPipelineLayout(appState->vulkan.device, hlsPlayer->pipelineLayout, NULL);
}

bool avdSceneHLSPlayerLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    *progress      = 1.0f;
    *statusMessage = "HLS Player scene loaded successfully.";
    return true;
}

void avdSceneHLSPlayerInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(event != NULL);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        } else if (event->key.key == GLFW_KEY_P && event->key.action == GLFW_PRESS) {
            AVD_LOG_INFO("P pressed - would start playing HLS stream if implemented %s.", glfwGetClipboardString(appState->window.window));
        }
    } else if (event->type == AVD_INPUT_EVENT_DRAG_N_DROP) {
        if (event->dragNDrop.count > 0) {
            AVD_LOG_INFO("Trying to load sources from: %s", event->dragNDrop.paths[0]);
            __avdSceneHLSPlayerLoadSourcesFromPath(__avdSceneGetTypePtr(scene), event->dragNDrop.paths[0]);
        }
    }
}

bool avdSceneHLSPlayerUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    if (!hlsPlayer->isSupported) {
        return true;
    }

    AVD_CHECK(__avdSceneHLSPlayerUpdateSources(appState, hlsPlayer));
    AVD_CHECK(__avdSceneHLSPlayerRecieveReadySegments(appState, hlsPlayer));

    return true;
}

bool avdSceneHLSPlayerRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    VkCommandBuffer commandBuffer = avdVulkanRendererGetCurrentCmdBuffer(&appState->renderer);
    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&hlsPlayer->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&hlsPlayer->info, &infoWidth, &infoHeight);

    if (hlsPlayer->isSupported) {
        AVD_HLSPlayerPushConstants pushConstants = {
            .activeSources = __avdSceneHLSPlayerUpdateActiveSources(hlsPlayer),
        };
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hlsPlayer->pipeline);
        vkCmdPushConstants(commandBuffer, hlsPlayer->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hlsPlayer->pipelineLayout, 0, 1, &appState->vulkan.bindlessDescriptorSet, 0, NULL);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    }

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &hlsPlayer->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &hlsPlayer->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}
