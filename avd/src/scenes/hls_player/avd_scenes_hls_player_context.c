#include "audio/avd_audio_core.h"
#include "audio/avd_audio_streaming_player.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "pico/picoPerf.h"
#include "pico/picoStream.h"
#include "pico/picoTime.h"
#include "scenes/hls_player/avd_scene_hls_player_context.h"
#include "scenes/hls_player/avd_scene_hls_player_segment_store.h"
#include "scenes/hls_player/avd_scene_hls_stream.h"
#include "scenes/hls_player/avd_scene_hls_worker_pool.h"
#include "vulkan/avd_vulkan_video.h"
#include "vulkan/video/avd_vulkan_video_decoder.h"
#include "vulkan/video/avd_vulkan_video_h264_data.h"
#include <math.h>

static bool __avdSceneHLSPlayerContextInitVideo(
    AVD_Vulkan *vulkan,
    AVD_Audio *audio,
    AVD_SceneHLSPlayerContext *context,
    AVD_HLSSegmentAVData avData)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    context->videoDataStream = avdHLSStreamCreate();
    if (!context->videoDataStream) {
        AVD_LOG_ERROR("Failed to create HLS stream for HLS player context");
        return false;
    }

    if (!avdHLSStreamAppendData(context->videoDataStream, avData.h264Buffer, avData.h264Size)) {
        AVD_LOG_ERROR("Failed to append data to HLS stream for HLS player context");
        picoStreamDestroy(context->videoDataStream);
        return false;
    }

    AVD_H264VideoLoadParams loadParams = {0};
    AVD_CHECK(avdH264VideoLoadParamsDefault(vulkan, &loadParams));

    AVD_H264Video *h264Video = NULL;
    if (!avdH264VideoLoadFromStream(context->videoDataStream, &loadParams, &h264Video)) {
        AVD_LOG_ERROR("Failed to initialize H264 video from buffer for HLS player context");
        avdSceneHLSPlayerContextDestroy(vulkan, audio, context);
        return false;
    }

    static char videoLabel[64];
    sprintf(videoLabel, "HLSPlayer/%zu", avData.source);

    if (!avdVulkanVideoDecoderCreate(vulkan, &context->videoPlayer, h264Video, videoLabel)) {
        AVD_LOG_ERROR("Failed to initialize Vulkan video decoder for HLS player context");
        avdSceneHLSPlayerContextDestroy(vulkan, audio, context);
        return false;
    }

    return true;
}

static bool __avdSceneHLSPlayerContextInitAudio(
    AVD_Vulkan *vulkan,
    AVD_Audio *audio,
    AVD_SceneHLSPlayerContext *context,
    AVD_HLSSegmentAVData avData)
{
    if (!avdAudioStreamingPlayerInit(audio, &context->audioPlayer, 4)) {
        AVD_LOG_ERROR("Failed to initialize audio streaming player for HLS player context");
        avdSceneHLSPlayerContextDestroy(vulkan, audio, context);
        return false;
    }

    if (!avdAudioStreamingPlayerAddChunk(&context->audioPlayer, avData.aacBuffer, avData.aacSize)) {
        AVD_LOG_ERROR("Failed to add audio chunk to streaming player in HLS player context");
        avdSceneHLSPlayerContextDestroy(vulkan, audio, context);
        return false;
    }

    if (!avdAudioStreamingPlayerPlay(&context->audioPlayer)) {
        AVD_LOG_ERROR("Failed to start audio streaming player for HLS player context");
        avdSceneHLSPlayerContextDestroy(vulkan, audio, context);
        return false;
    }

    return true;
}

static bool __avdSceneHLSPlayerContextInit(
    AVD_Vulkan *vulkan,
    AVD_Audio *audio,
    AVD_SceneHLSPlayerContext *context,
    AVD_HLSSegmentAVData avData)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    memset(context, 0, sizeof(AVD_SceneHLSPlayerContext));

    AVD_CHECK(avdHLSSegmentStoreInit(&context->segmentStore));
    AVD_CHECK(__avdSceneHLSPlayerContextInitVideo(vulkan, audio, context, avData));
    AVD_CHECK(__avdSceneHLSPlayerContextInitAudio(vulkan, audio, context, avData));

    context->sourceIndex             = avData.source;
    context->currentSegment          = avData;
    context->currentSegmentPlayTime  = 0.0f;
    context->currentSegmentStartTime = -100000.0f;

    context->initialized = true;
    return true;
}

static bool __avdSceneHLSPlayerContextSwitchToNextSegment(
    AVD_Vulkan *vulkan,
    AVD_Audio *audio,
    AVD_SceneHLSPlayerContext *context)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    // move to next segment
    AVD_Size nextSegmentId = {0};
    if (!avdHLSSegmentStoreFindNextSegment(
            &context->segmentStore,
            context->currentSegment.source,
            context->currentSegment.segmentId,
            &nextSegmentId)) {
        // no next segment yet, just wait
        return true;
    }

    AVD_LOG_INFO("HLS Player context switching to next segment %u at time %.3f", nextSegmentId, context->currentSegmentStartTime + context->currentSegmentPlayTime);

    avdHLSSegmentAVDataFree(&context->currentSegment);

    AVD_HLSSegmentAVData nextSegment = {0};
    AVD_CHECK(avdHLSSegmentStoreAcquire(
        &context->segmentStore,
        context->sourceIndex,
        nextSegmentId,
        &nextSegment));
    context->currentSegment                = nextSegment;
    context->currentSegmentTargetFramerate = avdH264VideoCountFrames(nextSegment.h264Buffer, nextSegment.h264Size) / nextSegment.duration;

    AVD_CHECK(avdAudioStreamingPlayerClearBuffers(&context->audioPlayer)); // NOTE: this isnt really needed
    AVD_CHECK(avdAudioStreamingPlayerAddChunk(&context->audioPlayer, nextSegment.aacBuffer, nextSegment.aacSize));

    // maybe clear here??
    AVD_CHECK(avdHLSStreamAppendData(
        context->videoDataStream,
        nextSegment.h264Buffer,
        nextSegment.h264Size));
    AVD_LOG_DEBUG("Appended %.3f seconds of video data to stream", nextSegment.duration);

    return true;
}

static bool __avdSceneHLSPlayerContextDecodeVideoFrames(
    AVD_Vulkan *vulkan,
    AVD_Audio *audio,
    AVD_SceneHLSPlayerContext *context)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    if (avdVulkanVideoDecoderChunkHasFrames(&context->videoPlayer)) {
        AVD_CHECK(avdVulkanVideoDecoderTryDecodeFrames(vulkan, &context->videoPlayer, true));
    } else {
        AVD_H264VideoLoadParams params = {0};
        AVD_CHECK(avdH264VideoLoadParamsDefault(vulkan, &params));
        params.targetFramerate = context->currentSegmentTargetFramerate;
        bool eof               = false;
        AVD_CHECK(avdVulkanVideoDecoderNextChunk(vulkan, &context->videoPlayer, &params, &eof));
        context->currentFrame = NULL;
        if (context->videoPlayer.h264Video->currentChunk.numNalUnitsParsed == 0 && eof) {
            AVD_LOG_DEBUG_DEBOUNCED(1000, "out of video data, segtime=%.3f seg dur=%.3f, currTime: %.3f", context->currentSegmentPlayTime, context->currentSegment.duration, context->currentSegmentStartTime + context->currentSegmentPlayTime);
        }
    }

    return true;
}

void avdSceneHLSPlayerContextDestroy(AVD_Vulkan *vulkan, AVD_Audio *audio, AVD_SceneHLSPlayerContext *context)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    if (!context->initialized) {
        memset(context, 0, sizeof(AVD_SceneHLSPlayerContext));
        return;
    }

    avdHLSSegmentAVDataFree(&context->currentSegment);

    avdHLSSegmentStoreDestroy(&context->segmentStore);

    if (context->videoPlayer.initialized) {
        avdVulkanVideoDecoderDestroy(vulkan, &context->videoPlayer);
    }
    if (context->audioPlayer.initialized) {
        avdAudioStreamingPlayerShutdown(&context->audioPlayer);
    }
    memset(context, 0, sizeof(AVD_SceneHLSPlayerContext));
}

bool avdSceneHLSPlayerContextAddSegment(
    AVD_Vulkan *vulkan,
    AVD_Audio *audio,
    AVD_SceneHLSPlayerContext *context,
    AVD_HLSSegmentAVData avData)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    if (!context->initialized) {
        // for first segment, just init
        AVD_CHECK(__avdSceneHLSPlayerContextInit(vulkan, audio, context, avData));
        return true;
    }

    AVD_CHECK_MSG(
        avData.source == context->sourceIndex,
        "HLS Player context: segment source index %zu does not match context source index %zu",
        avData.source,
        context->sourceIndex);

    bool isSegmentOld  = avData.segmentId <= context->currentSegment.segmentId;
    bool segmentExists = avdHLSSegmentStoreHasSegment(&context->segmentStore, avData.source, avData.segmentId);
    if (isSegmentOld || segmentExists) {
        avdHLSSegmentAVDataFree(&avData);
        return true;
    }

    AVD_LOG_VERBOSE("new segment: src=%zu seg=%zu duration=%.3f now playing: %zu", avData.source, avData.segmentId, avData.duration, context->currentSegment.segmentId);
    AVD_CHECK(avdHLSSegmentStoreAdd(&context->segmentStore, avData));

    return true;
}

bool avdSceneHLSPlayerContextIsFed(AVD_SceneHLSPlayerContext *context)
{
    AVD_ASSERT(context != NULL);

    if (!context->initialized) {
        return false;
    }

    return avdHLSSegmentStoreGetLoadedSegmentCount(&context->segmentStore, context->currentSegment.source) > 0;
}

bool avdSceneHLSPlayerContextTryAcquireFrame(
    AVD_SceneHLSPlayerContext *context,
    AVD_VulkanVideoDecodedFrame **outFrame)
{
    AVD_ASSERT(context != NULL);
    AVD_ASSERT(outFrame != NULL);

    AVD_VulkanVideoDecodedFrame *oldFrame = context->currentFrame;

    (void)avdVulkanVideoDecoderTryAcquireFrame(
        &context->videoPlayer,
        context->currentSegmentStartTime + context->currentSegmentPlayTime,
        &context->currentFrame);

    *outFrame = context->currentFrame;
    return oldFrame != context->currentFrame;
}

AVD_Float avdSceneHLSPlayerContextGetTime(AVD_SceneHLSPlayerContext *context)
{
    AVD_ASSERT(context != NULL);

    if (!context->initialized) {
        return 0.0f;
    }

    return context->currentSegmentStartTime + context->currentSegmentPlayTime;
}

bool avdSceneHLSPlayerContextUpdate(AVD_Vulkan *vulkan, AVD_Audio *audio, AVD_SceneHLSPlayerContext *context, AVD_Float time)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    if (!context->initialized) {
        return true;
    }

    // check if current chunk is still playing, else switch to next (with some leeway)
    if (time - context->currentSegmentStartTime > context->currentSegment.duration) {
        AVD_CHECK(__avdSceneHLSPlayerContextSwitchToNextSegment(vulkan, audio, context));
        context->currentSegmentPlayTime             = 0.0f;
        context->currentSegmentStartTime            = time;
        context->videoPlayer.timestampSecondsOffset = time; // NOTE: I know that this is kinda messy, I will clean it up later...
    } else {
        context->currentSegmentPlayTime = time - context->currentSegmentStartTime;
        if (!avdVulkanVideoDecoderHasFrameForTime(&context->videoPlayer, time)) {
            AVD_CHECK(__avdSceneHLSPlayerContextDecodeVideoFrames(vulkan, audio, context));
        }
    }

    AVD_CHECK(avdAudioStreamingPlayerUpdate(&context->audioPlayer));
    AVD_CHECK(avdVulkanVideoDecoderUpdate(vulkan, &context->videoPlayer));

    return true;
}

AVD_Size avdSceneHLSPlayerContextGetCurrentlyPlayingSegmentId(AVD_SceneHLSPlayerContext *context)
{
    AVD_ASSERT(context != NULL);

    if (!context->initialized) {
        return 0;
    }

    return context->currentSegment.segmentId;
}