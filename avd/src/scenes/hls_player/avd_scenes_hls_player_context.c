#include "audio/avd_audio_core.h"
#include "audio/avd_audio_streaming_player.h"
#include "core/avd_base.h"
#include "core/avd_utils.h"
#include "pico/picoStream.h"
#include "scenes/hls_player/avd_scene_hls_player_context.h"
#include "scenes/hls_player/avd_scene_hls_stream.h"
#include "scenes/hls_player/avd_scene_hls_worker_pool.h"
#include "vulkan/avd_vulkan_video.h"
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

    if (!avdVulkanVideoDecoderCreate(vulkan, &context->videoPlayer, h264Video)) {
        AVD_LOG_ERROR("Failed to initialize Vulkan video decoder for HLS player context");
        avdSceneHLSPlayerContextDestroy(vulkan, audio, context);
        return false;
    }

    context->videoHungry = false;

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

    AVD_CHECK(__avdSceneHLSPlayerContextInitVideo(vulkan, audio, context, avData));
    AVD_CHECK(__avdSceneHLSPlayerContextInitAudio(vulkan, audio, context, avData));

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

    if (!avdAudioStreamingPlayerAddChunk(&context->audioPlayer, avData.aacBuffer, avData.aacSize)) {
        AVD_LOG_ERROR("Failed to add audio chunk to streaming player in HLS player context");
        return false;
    }

    return true;
}

static bool __avdSceneHLSPlayerContextAddSegmentVideo(
    AVD_Vulkan *vulkan,
    AVD_SceneHLSPlayerContext *context,
    AVD_HLSSegmentAVData avData)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(context != NULL);

    if (!avdHLSStreamAppendData(context->videoDataStream, avData.h264Buffer, avData.h264Size)) {
        AVD_LOG_ERROR("Failed to append data to HLS stream for HLS player context");
        return false;
    }

    return true;
}

static bool __avdVulkanVideoDecoderUpdate(
    AVD_SceneHLSPlayerContext *context,
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDecoder *decoder)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(decoder != NULL);

    if (!decoder->initialized) {
        return true;
    }

    if (avdVulkanVideoDecoderGetNumDecodedFrames(decoder) < AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES) {
        // we have room for more decoded frames
        if (!avdVulkanVideoDecoderChunkHasFrames(decoder)) {
            // try to decode more frames
            AVD_H264VideoLoadParams loadParams = {0};
            AVD_CHECK(avdH264VideoLoadParamsDefault(vulkan, &loadParams));
            loadParams.targetFramerate = context->videoFramerate;
            bool eof                   = false;
            AVD_CHECK(avdVulkanVideoDecoderNextChunk(vulkan, decoder, &loadParams, &eof));
            if (decoder->h264Video->currentChunk.numNalUnitsParsed == 0 && eof) {
                // no more data to decode
                context->videoHungry = true;
            }

            if (decoder->h264Video->currentChunk.numNalUnitsParsed > 0) {
                AVD_Float audioTimeMs = 0.0f;
                avdAudioStreamingPlayerGetTimePlayedMs(
                    &context->audioPlayer,
                    &audioTimeMs);
                AVD_LOG_WARN(
                    "Video Time: %.3f - Audio Time: %.3f",
                    decoder->h264Video->currentChunk.timestampSeconds,
                    audioTimeMs / 1000.0f);
                context->videoHungry = false;
            }
        } else {

            AVD_CHECK(
                avdVulkanVideoDecoderDecodeFrame(
                    vulkan,
                    decoder,
                    VK_NULL_HANDLE,
                    VK_NULL_HANDLE));
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

    AVD_Float videoFramerate = (AVD_Float)avdH264VideoCountFrames(avData.h264Buffer, avData.h264Size) / avData.duration;
    if (fabsf(context->videoFramerate - videoFramerate) > 0.01f) {
        AVD_LOG_WARN(
            "HLS Player context video framerate changed from %.3f to %.3f fps",
            context->videoFramerate,
            videoFramerate);
        context->videoFramerate = videoFramerate;
    }

    if (!context->initialized) {
        AVD_CHECK(__avdSceneHLSPlayerContextInit(vulkan, audio, context, avData));
        return true;
    }

    // this part is temporary
    AVD_CHECK(__avdSceneHLSPlayerContextAddSegmentAudio(audio, context, avData));
    AVD_CHECK(__avdSceneHLSPlayerContextAddSegmentVideo(vulkan, context, avData));

    AVD_LOG_VERBOSE("new segment: src=%zu seg=%zu duration=%.3f", avData.source, avData.segmentId, avData.duration);

    avdHLSSegmentAVDataFree(&avData);
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

    AVD_CHECK(avdAudioStreamingPlayerUpdate(&context->audioPlayer));
    AVD_CHECK(__avdVulkanVideoDecoderUpdate(context, vulkan, &context->videoPlayer));

    return true;
}

bool avdSceneHLSPlayerContextIsFed(AVD_SceneHLSPlayerContext *context)
{
    AVD_ASSERT(context != NULL);

    if (!context->initialized) {
        return false;
    }

    AVD_Bool audioFed = avdAudioStreamingPlayerIsFed(&context->audioPlayer);

    if (audioFed == context->videoHungry) {
        AVD_LOG_ERROR("Audio/Video sync issue in HLS Player context: audioFed=%s videoFed=%s",
                      audioFed ? "true" : "false",
                      !context->videoHungry ? "true" : "false");
    }

    return audioFed && !context->videoHungry;
}