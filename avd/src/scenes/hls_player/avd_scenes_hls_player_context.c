#include "audio/avd_audio_core.h"
#include "core/avd_base.h"
#include "core/avd_utils.h"
#include "pico/picoStream.h"
#include "scenes/hls_player/avd_scene_hls_player_context.h"
#include "scenes/hls_player/avd_scene_hls_stream.h"
#include "scenes/hls_player/avd_scene_hls_worker_pool.h"
#include "vulkan/avd_vulkan_video.h"

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

    if (!avdVulkanVideoDecoderCreate(vulkan, &context->videoPlayer, h264Video)) {
        AVD_LOG_ERROR("Failed to initialize Vulkan video decoder for HLS player context");
        avdSceneHLSPlayerContextDestroy(vulkan, audio, context);
        return false;
    }

    if (!avdAudioStreamingPlayerInit(audio, &context->audioPlayer, 4)) {
        AVD_LOG_ERROR("Failed to initialize audio streaming player for HLS player context");
        avdSceneHLSPlayerContextDestroy(vulkan, audio, context);
        return false;
    }

    context->initialized = true;
    return true;
}

static bool __avdSceneHLSPlayerContextAddSegmentAudio(
    AVD_Audio *audio,
    AVD_SceneHLSPlayerContext *context,
    AVD_HLSSegmentAVData avData)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    if (!avdAudioStreamingPlayerAddChunk(audio, &context->audioPlayer, avData.aacBuffer, avData.aacSize)) {
        AVD_LOG_ERROR("Failed to add audio chunk to streaming player in HLS player context");
        return false;
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

    if (context->videoPlayer.initialized) {
        avdVulkanVideoDecoderDestroy(vulkan, &context->videoPlayer);
    }
    if (context->audioPlayer.initialized) {
        avdAudioStreamingPlayerShutdown(audio, &context->audioPlayer);
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
        AVD_CHECK(__avdSceneHLSPlayerContextInit(vulkan, audio, context, avData));
    }

    // this part is temporary
    AVD_CHECK(__avdSceneHLSPlayerContextAddSegmentAudio(audio, context, avData));

    avdHLSSegmentAVDataFree(&avData);

    AVD_LOG_VERBOSE("new segment: src=%u seg=%u duration=%.3f", avData.source, avData.segmentId, avData.duration);

    return true;
}

bool avdSceneHLSPlayerContextUpdate(AVD_Vulkan *vulkan, AVD_Audio *audio, AVD_SceneHLSPlayerContext *context)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(context != NULL);

    if (!context->initialized) {
        return true;
    }

    // AVD_CHECK(avdVulkanVideoDecoderDecodeFrame(vulkan, &context->videoPlayer, VK_NULL_HANDLE, VK_NULL_HANDLE));
    AVD_CHECK(avdAudioStreamingPlayerUpdate(audio, &context->audioPlayer));

    return true;
}