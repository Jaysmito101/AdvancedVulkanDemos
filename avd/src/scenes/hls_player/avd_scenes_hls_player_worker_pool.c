#include "pico/picoPerf.h"
#include "scenes/hls_player/avd_hls_worker_pool.h"

#include "core/avd_base.h"
#include "core/avd_utils.h"
#include "pico/picoM3U8.h"
#include "pico/picoMpegTS.h"
#include "scenes/hls_player/avd_scenes_hls_player.h"
#include "vulkan/avd_vulkan_video.h"

#include <stdlib.h>
#include <string.h>

static void __avdHLSWorkerPoolFreeDemuxPayload(void *payloadRaw, void *context)
{
    (void)context;

    AVD_HLSDemuxTaskPayload *payload = (AVD_HLSDemuxTaskPayload *)payloadRaw;
    AVD_ASSERT(payload != NULL);

    if (payload->data) {
        free(payload->data);
        payload->data     = NULL;
        payload->dataSize = 0;
    }
}

static void __avdHLSWorkerPoolFreeReadyPayload(void *payloadRaw, void *context)
{
    (void)context;

    AVD_HLSReadyPayload *payload = (AVD_HLSReadyPayload *)payloadRaw;
    AVD_ASSERT(payload != NULL);

    if (payload->video) {
        avdH264VideoDestroy(payload->video);
        payload->video = NULL;
    }
}

static void __avdHLSSourceDownloadWorker(void *arg)
{
    AVD_HLSWorkerPool *pool     = (AVD_HLSWorkerPool *)arg;
    pool->sourceDownloadRunning = true;

    AVD_LOG_INFO("HLS Source Download Worker started with thread ID: %llu.", picoThreadGetCurrentId());

    AVD_HLSSourceTaskPayload sourcePayload = {0};
    AVD_HLSMediaTaskPayload mediaPayload   = {0};
    picoM3U8Playlist playlist              = NULL;

    while (pool->sourceDownloadRunning) {
        if (!picoThreadChannelReceive(pool->sourceDownloadChannel, &sourcePayload, 200)) {
            continue;
        }

        const char *sourceUrl = avdHLSURLPoolGet(pool->urlPool, sourcePayload.sourceUrlHash);
        if (!sourceUrl) {
            AVD_LOG_ERROR("Failed to get source URL from pool for hash 0x%08X", sourcePayload.sourceUrlHash);
            continue;
        }

        char *data = NULL;

        if (!avdCurlFetchStringContent(sourceUrl, &data, NULL)) {
            AVD_LOG_ERROR("Failed to fetch HLS playlist for source: %s", sourceUrl);
            goto cleanup;
        }

        playlist = NULL;
        if (picoM3U8PlaylistParse(data, (uint32_t)strlen(data), &playlist) != PICO_M3U8_RESULT_SUCCESS) {
            AVD_LOG_ERROR("Failed to parse HLS playlist for source: %s", sourceUrl);
            goto cleanup;
        }

        if (playlist->type != PICO_M3U8_PLAYLIST_TYPE_MEDIA) {
            AVD_LOG_ERROR("Master playlists are not supported yet: %s", sourceUrl);
            goto cleanup;
        }

        if (playlist->media.mediaSegmentCount == 0) {
            AVD_LOG_WARN("No media segments found in playlist: %s", sourceUrl);
            goto cleanup;
        }

        mediaPayload.sourcesHash       = sourcePayload.sourcesHash;
        mediaPayload.sourceIndex       = sourcePayload.sourceIndex;
        mediaPayload.refreshIntervalMs = (AVD_Float)playlist->media.mediaSegments[0].duration;

        char resolvedUrl[1024];
        for (AVD_UInt32 i = 0; i < playlist->media.mediaSegmentCount; i++) {
            picoM3U8MediaSegment segment = &playlist->media.mediaSegments[i];
            avdResolveRelativeURL(resolvedUrl, sizeof(resolvedUrl), sourceUrl, segment->uri);

            mediaPayload.segmentId = i + playlist->media.mediaSequence;
            mediaPayload.duration  = segment->duration;
            if (!avdHLSURLPoolInsert(pool->urlPool, resolvedUrl, &mediaPayload.urlHash)) {
                AVD_LOG_ERROR("Failed to intern URL into pool: %s", resolvedUrl);
                continue;
            }

            if (!avdHLSSegmentStoreReserve(pool->segmentStore, sourcePayload.sourceIndex, mediaPayload.segmentId)) {
                continue;
            }

            if (!picoThreadChannelSend(pool->mediaDownloadChannel, &mediaPayload)) {
                AVD_LOG_ERROR("Failed to send media download task");
            }
        }

    cleanup:
        if (playlist) {
            picoM3U8PlaylistDestroy(playlist);
        }
        if (data) {
            free(data);
        }
    }

    AVD_LOG_INFO("HLS Source Download Worker stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

static void __avdHLSMediaDownloadWorker(void *arg)
{
    AVD_HLSWorkerPool *pool    = (AVD_HLSWorkerPool *)arg;
    pool->mediaDownloadRunning = true;

    AVD_LOG_INFO("HLS Media Download Worker started with thread ID: %llu.", picoThreadGetCurrentId());

    AVD_HLSMediaTaskPayload mediaPayload = {0};
    AVD_HLSDemuxTaskPayload demuxPayload = {0};

    while (pool->mediaDownloadRunning) {
        if (!picoThreadChannelReceive(pool->mediaDownloadChannel, &mediaPayload, 200)) {
            continue;
        }

        demuxPayload.segmentId   = mediaPayload.segmentId;
        demuxPayload.sourceIndex = mediaPayload.sourceIndex;
        demuxPayload.duration    = mediaPayload.duration;
        demuxPayload.sourcesHash = mediaPayload.sourcesHash;
        demuxPayload.data        = NULL;
        demuxPayload.dataSize    = 0;

        if (avdHLSMediaCacheQuery(pool->mediaCache, mediaPayload.urlHash, &demuxPayload.data, &demuxPayload.dataSize)) {
            if (!picoThreadChannelSend(pool->mediaDemuxChannel, &demuxPayload)) {
                AVD_LOG_ERROR("Failed to send media demux task (cached)");
                free(demuxPayload.data);
            }
            continue;
        }

        const char *segmentUrl = avdHLSURLPoolGet(pool->urlPool, mediaPayload.urlHash);
        if (!segmentUrl) {
            AVD_LOG_ERROR("Failed to get URL from pool for hash 0x%08X", mediaPayload.urlHash);
            continue;
        }

        picoPerfTime downloadStartTime = picoPerfNow();
        if (!avdCurlDownloadToMemory(segmentUrl, (void **)&demuxPayload.data, &demuxPayload.dataSize)) {
            AVD_LOG_ERROR("Failed to download media segment from url: %s", segmentUrl);
            continue;
        }
        picoPerfTime downloadEndTime = picoPerfNow();
        // AVD_LOG_VERBOSE("Downloading segment %u took %lf ms", demuxPayload.segmentId, picoPerfDurationMilliseconds(downloadStartTime, downloadEndTime));

        avdHLSMediaCacheInsert(pool->mediaCache, mediaPayload.urlHash, demuxPayload.data, demuxPayload.dataSize);

        if (!picoThreadChannelSend(pool->mediaDemuxChannel, &demuxPayload)) {
            AVD_LOG_ERROR("Failed to send media demux task");
            free(demuxPayload.data);
        }
    }

    AVD_LOG_INFO("HLS Media Download Worker stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

static void __avdHLSMediaDemuxWorker(void *arg)
{
    AVD_HLSWorkerPool *pool = (AVD_HLSWorkerPool *)arg;
    pool->mediaDemuxRunning = true;

    AVD_LOG_INFO("HLS Media Demux Worker started with thread ID: %llu.", picoThreadGetCurrentId());

    AVD_HLSDemuxTaskPayload demuxPayload = {0};
    AVD_HLSReadyPayload readyPayload     = {0};

    while (pool->mediaDemuxRunning) {
        if (!picoThreadChannelReceive(pool->mediaDemuxChannel, &demuxPayload, 200)) {
            continue;
        }

        picoPerfTime demuxStartTime = picoPerfNow();

        char *h264Buffer  = NULL;
        picoMpegTS mpegts = picoMpegTSCreate(false);
        if (!mpegts) {
            AVD_LOG_ERROR("Failed to create MPEG-TS parser for segment %u", demuxPayload.segmentId);
            goto cleanup;
        }

        if (picoMpegTSAddBuffer(mpegts, (uint8_t *)demuxPayload.data, demuxPayload.dataSize) != PICO_MPEGTS_RESULT_SUCCESS) {
            AVD_LOG_ERROR("Failed to parse MPEG-TS buffer for segment %u", demuxPayload.segmentId);
            goto cleanup;
        }

        size_t pesPacketCount           = 0;
        picoMpegTSPESPacket *pesPackets = picoMpegTSGetPESPackets(mpegts, &pesPacketCount);

        AVD_Size totalH264Size = 0;
        for (AVD_Size i = 0; i < pesPacketCount; i++) {
            picoMpegTSPESPacket pesPacket = pesPackets[i];
            if (picoMpegTSGetPMSStreamByPID(mpegts, pesPacket->head.pid)->streamType == PICO_MPEGTS_STREAM_TYPE_H264) {
                totalH264Size += pesPacket->dataLength;
            } else if (picoMpegTSIsStreamIDVideo(pesPacket->head.streamId)) {
                AVD_LOG_WARN("Found video PES packet with video, but stream type is not H.264 for segment %u", demuxPayload.segmentId);
            }
        }

        if (totalH264Size == 0) {
            AVD_LOG_ERROR("No H.264 video data found in segment %u", demuxPayload.segmentId);
            goto cleanup;
        }

        h264Buffer = (char *)AVD_MALLOC(totalH264Size);
        if (!h264Buffer) {
            AVD_LOG_ERROR("Failed to allocate H.264 buffer for segment %u", demuxPayload.segmentId);
            goto cleanup;
        }

        size_t h264Offset = 0;
        for (AVD_Size i = 0; i < pesPacketCount; i++) {
            picoMpegTSPESPacket pesPacket = pesPackets[i];
            if (picoMpegTSGetPMSStreamByPID(mpegts, pesPacket->head.pid)->streamType == PICO_MPEGTS_STREAM_TYPE_H264) {
                memcpy(h264Buffer + h264Offset, pesPacket->data, pesPacket->dataLength);
                h264Offset += pesPacket->dataLength;
            }
        }

        AVD_H264Video *video           = NULL;
        AVD_H264VideoLoadParams params = {0};
        avdH264VideoLoadParamsDefault(pool->vulkan, &params);

        if (!avdH264VideoLoadFromBuffer((uint8_t *)h264Buffer, totalH264Size, true, &params, &video)) {
            AVD_LOG_ERROR("Failed to load H.264 video for segment %u", demuxPayload.segmentId);
            goto cleanup;
        }

        h264Buffer = NULL; // ownership transferred to avdH264VideoLoadFromBuffer

        readyPayload.segmentId   = demuxPayload.segmentId;
        readyPayload.sourceIndex = demuxPayload.sourceIndex;
        readyPayload.duration    = demuxPayload.duration;
        readyPayload.video       = video;
        readyPayload.sourcesHash = demuxPayload.sourcesHash;

        picoPerfTime demuxEndTime = picoPerfNow();
        // AVD_LOG_VERBOSE("Demuxing segment %u took %lf ms", demuxPayload.segmentId, picoPerfDurationMilliseconds(demuxStartTime, demuxEndTime));

        if (!picoThreadChannelSend(pool->mediaReadyChannel, &readyPayload)) {
            AVD_LOG_ERROR("Failed to send ready segment %u", demuxPayload.segmentId);
            avdH264VideoDestroy(video);
        }

    cleanup:
        if (h264Buffer) {
            free(h264Buffer);
        }
        if (mpegts) {
            picoMpegTSDestroy(mpegts);
        }
        if (demuxPayload.data) {
            free(demuxPayload.data);
        }
    }

    AVD_LOG_INFO("HLS Media Demux Worker stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

bool avdHLSWorkerPoolInit(AVD_HLSWorkerPool *pool, AVD_HLSURLPool *urlPool, AVD_HLSMediaCache *mediaCache, AVD_HLSSegmentStore *segmentStore, struct AVD_Vulkan *vulkan, struct AVD_SceneHLSPlayer *scene)
{
    AVD_ASSERT(pool != NULL);
    AVD_ASSERT(urlPool != NULL);
    AVD_ASSERT(mediaCache != NULL);
    AVD_ASSERT(segmentStore != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(scene != NULL);

    memset(pool, 0, sizeof(AVD_HLSWorkerPool));

    pool->urlPool      = urlPool;
    pool->mediaCache   = mediaCache;
    pool->segmentStore = segmentStore;
    pool->vulkan       = vulkan;
    pool->scene        = scene;

    pool->sourceDownloadChannel = picoThreadChannelCreateUnbounded(sizeof(AVD_HLSSourceTaskPayload));
    AVD_CHECK_MSG(pool->sourceDownloadChannel != NULL, "Failed to create source download channel");

    pool->mediaDownloadChannel = picoThreadChannelCreateUnbounded(sizeof(AVD_HLSMediaTaskPayload));
    AVD_CHECK_MSG(pool->mediaDownloadChannel != NULL, "Failed to create media download channel");

    pool->mediaDemuxChannel = picoThreadChannelCreateUnbounded(sizeof(AVD_HLSDemuxTaskPayload));
    AVD_CHECK_MSG(pool->mediaDemuxChannel != NULL, "Failed to create media demux channel");
    picoThreadChannelSetItemDestructor(pool->mediaDemuxChannel, __avdHLSWorkerPoolFreeDemuxPayload, NULL);

    pool->mediaReadyChannel = picoThreadChannelCreateUnbounded(sizeof(AVD_HLSReadyPayload));
    AVD_CHECK_MSG(pool->mediaReadyChannel != NULL, "Failed to create media ready channel");
    picoThreadChannelSetItemDestructor(pool->mediaReadyChannel, __avdHLSWorkerPoolFreeReadyPayload, NULL);

    for (AVD_Size i = 0; i < AVD_HLS_WORKER_NUM_SOURCE_WORKERS; i++) {
        pool->sourceDownloadWorkers[i] = picoThreadCreate(__avdHLSSourceDownloadWorker, pool);
        AVD_CHECK_MSG(pool->sourceDownloadWorkers[i] != NULL, "Failed to create source download worker");
    }

    for (AVD_Size i = 0; i < AVD_HLS_WORKER_NUM_MEDIA_DOWNLOAD_WORKERS; i++) {
        pool->mediaDownloadWorkers[i] = picoThreadCreate(__avdHLSMediaDownloadWorker, pool);
        AVD_CHECK_MSG(pool->mediaDownloadWorkers[i] != NULL, "Failed to create media download worker");
    }

    for (AVD_Size i = 0; i < AVD_HLS_WORKER_NUM_MEDIA_DEMUX_WORKERS; i++) {
        pool->mediaDemuxWorkers[i] = picoThreadCreate(__avdHLSMediaDemuxWorker, pool);
        AVD_CHECK_MSG(pool->mediaDemuxWorkers[i] != NULL, "Failed to create media demux worker");
    }

    return true;
}

void avdHLSWorkerPoolDestroy(AVD_HLSWorkerPool *pool)
{
    AVD_ASSERT(pool != NULL);

    pool->sourceDownloadRunning = false;
    pool->mediaDownloadRunning  = false;
    pool->mediaDemuxRunning     = false;

    for (AVD_Size i = 0; i < AVD_HLS_WORKER_NUM_SOURCE_WORKERS; i++) {
        if (pool->sourceDownloadWorkers[i]) {
            picoThreadDestroy(pool->sourceDownloadWorkers[i]);
            pool->sourceDownloadWorkers[i] = NULL;
        }
    }

    for (AVD_Size i = 0; i < AVD_HLS_WORKER_NUM_MEDIA_DOWNLOAD_WORKERS; i++) {
        picoThreadDestroy(pool->mediaDownloadWorkers[i]);
        pool->mediaDownloadWorkers[i] = NULL;
    }

    for (AVD_Size i = 0; i < AVD_HLS_WORKER_NUM_MEDIA_DEMUX_WORKERS; i++) {
        picoThreadDestroy(pool->mediaDemuxWorkers[i]);
        pool->mediaDemuxWorkers[i] = NULL;
    }

    picoThreadChannelDestroy(pool->sourceDownloadChannel);
    pool->sourceDownloadChannel = NULL;

    picoThreadChannelDestroy(pool->mediaDownloadChannel);
    pool->mediaDownloadChannel = NULL;

    picoThreadChannelDestroy(pool->mediaDemuxChannel);
    pool->mediaDemuxChannel = NULL;

    picoThreadChannelDestroy(pool->mediaReadyChannel);
    pool->mediaReadyChannel = NULL;
}

void avdHLSWorkerPoolFlush(AVD_HLSWorkerPool *pool)
{
    AVD_ASSERT(pool != NULL);

    AVD_HLSSourceTaskPayload sourcePayload = {0};
    while (picoThreadChannelTryReceive(pool->sourceDownloadChannel, &sourcePayload)) {
    }

    AVD_HLSMediaTaskPayload mediaPayload = {0};
    while (picoThreadChannelTryReceive(pool->mediaDownloadChannel, &mediaPayload)) {
    }

    AVD_HLSDemuxTaskPayload demuxPayload = {0};
    while (picoThreadChannelTryReceive(pool->mediaDemuxChannel, &demuxPayload)) {
        __avdHLSWorkerPoolFreeDemuxPayload(&demuxPayload, NULL);
    }

    AVD_HLSReadyPayload readyPayload = {0};
    while (picoThreadChannelTryReceive(pool->mediaReadyChannel, &readyPayload)) {
        __avdHLSWorkerPoolFreeReadyPayload(&readyPayload, NULL);
    }
}

bool avdHLSWorkerPoolSendSourceTask(AVD_HLSWorkerPool *pool, AVD_UInt32 sourceIndex, AVD_UInt32 sourcesHash, const char *sourceUrl)
{
    AVD_ASSERT(pool != NULL);
    AVD_ASSERT(sourceUrl != NULL);

    AVD_UInt32 sourceUrlHash = 0;
    if (!avdHLSURLPoolInsert(pool->urlPool, sourceUrl, &sourceUrlHash)) {
        AVD_LOG_ERROR("Failed to insert source URL into pool: %s", sourceUrl);
        return false;
    }

    AVD_HLSSourceTaskPayload payload = {
        .sourceIndex   = sourceIndex,
        .sourcesHash   = sourcesHash,
        .sourceUrlHash = sourceUrlHash,
    };

    return picoThreadChannelSend(pool->sourceDownloadChannel, &payload);
}

bool avdHLSWorkerPoolReceiveReadySegment(AVD_HLSWorkerPool *pool, AVD_HLSReadyPayload *outPayload)
{
    AVD_ASSERT(pool != NULL);
    AVD_ASSERT(outPayload != NULL);

    return picoThreadChannelTryReceive(pool->mediaReadyChannel, outPayload);
}
