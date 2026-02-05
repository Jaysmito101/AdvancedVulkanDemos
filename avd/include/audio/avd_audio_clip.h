#ifndef AVD_AUDIO_DATA_H
#define AVD_AUDIO_DATA_H

#include "core/avd_core.h"

typedef struct {
    void *samples;
    AVD_Size sampleCount;
    AVD_UInt32 sampleRate;
    AVD_UInt32 channels;
    AVD_UInt32 bitsPerSample;
    AVD_Bool stereo;
} AVD_AudioClip;

bool avdAudioClipFromFile(const char *filePath, AVD_AudioClip *outAudioData);
bool avdAudioClipFromMemory(const void *data, AVD_Size dataSize, AVD_AudioClip *outAudioData);
bool avdAudioClipFromRawPCM(
    const void *data,
    AVD_Size dataSize,
    AVD_UInt32 sampleRate,
    AVD_UInt32 channels,
    AVD_UInt32 bitsPerSample,
    AVD_AudioClip *outAudioData);
bool avdAudioClipSilent(
    AVD_UInt32 durationMs,
    AVD_UInt32 sampleRate,
    AVD_UInt32 channels,
    AVD_UInt32 bitsPerSample,
    AVD_AudioClip *outAudioData);
bool avdAudioClipFree(AVD_AudioClip *audioData);
AVD_Float avdAudioClipDurationMs(const AVD_AudioClip *audioData);
bool avdAudioClipSampleAtIndex(
    const AVD_AudioClip *audioData,
    AVD_Size index,
    AVD_Float *outSample);
bool avdAudioClipSampleAtTimeMs(
    const AVD_AudioClip *audioData,
    AVD_Float timeMs,
    AVD_Float *outSample);

#endif // AVD_AUDIO_DATA_H