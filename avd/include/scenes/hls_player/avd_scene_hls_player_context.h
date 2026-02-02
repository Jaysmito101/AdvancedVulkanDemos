#ifndef AVD_SCENE_HLS_PLAYER_CONTEXT_H
#define AVD_SCENE_HLS_PLAYER_CONTEXT_H

#include "audio/avd_audio_core.h"
#include "audio/avd_audio_streaming_player.h"
#include "avd_scene_hls_player_segment_store.h"
#include "core/avd_core.h"
#include "core/avd_types.h"
#include "pico/picoStream.h"
#include "scenes/hls_player/avd_scene_hls_worker_pool.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_video.h"

typedef struct {
    bool initialized;

    AVD_HLSSegmentStore segmentStore;

    AVD_HLSSegmentAVData currentSegment;
    AVD_Float currentSegmentPlayTime;
    AVD_Float currentSegmentStartTime;
    AVD_Float currentSegmentTargetFramerate;

    picoStream videoDataStream;
    AVD_VulkanVideoDecoder videoPlayer;
    AVD_AudioStreamingPlayer audioPlayer;
} AVD_SceneHLSPlayerContext;

void avdSceneHLSPlayerContextDestroy(AVD_Vulkan *vulkan, AVD_Audio *audio, AVD_SceneHLSPlayerContext *context);
bool avdSceneHLSPlayerContextAddSegment(
    AVD_Vulkan *vulkan,
    AVD_Audio *audio,
    AVD_SceneHLSPlayerContext *context,
    AVD_HLSSegmentAVData avData);
bool avdSceneHLSPlayerContextUpdate(AVD_Vulkan *vulkan, AVD_Audio *audio, AVD_SceneHLSPlayerContext *context, AVD_Float time);
bool avdSceneHLSPlayerContextIsFed(AVD_SceneHLSPlayerContext *context);

bool avdSceneHLSPlayerContextTryAcquireFrame(
    AVD_SceneHLSPlayerContext *context,
    AVD_VulkanVideoDecodedFrame **outFrame);
AVD_Size avdSceneHLSPlayerContextGetCurrentlyPlayingSegmentId(AVD_SceneHLSPlayerContext *context);
AVD_Float avdSceneHLSPlayerContextGetTime(AVD_SceneHLSPlayerContext *context);

#endif // AVD_SCENE_HLS_PLAYER_CONTEXT_H