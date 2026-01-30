#ifndef AVD_AUDIO_STREAMING_PLAYER_H
#define AVD_AUDIO_STREAMING_PLAYER_H

#include "audio/avd_audio_core.h"
#include "core/avd_types.h"

#ifndef AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS
#define AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS 8
#endif

#ifndef AVD_AUDIO_STREAMING_PLAYER_MAX_CHUNK_SIZE
#define AVD_AUDIO_STREAMING_PLAYER_MAX_CHUNK_SIZE (4 * 1024 * 1024) // 4 MB
#endif

typedef struct {
    AVD_Bool inUse;
    AVD_UInt32 lastUpdated;
    AVD_UInt8 data[AVD_AUDIO_STREAMING_PLAYER_MAX_CHUNK_SIZE];
    AVD_Size dataSize;
} AVD_AudioStreamingPlayerBufferChunk;

typedef struct {
    bool initialized;
    AVD_AudioBuffer buffers[AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS];
    AVD_Size bufferCount;

    AVD_AudioStreamingPlayerBufferChunk chunks[AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS];
    AVD_Size chunkCount;

    AVD_Size buffersProcessed;

    AVD_AudioSource source;
} AVD_AudioStreamingPlayer;

bool avdAudioStreamingPlayerInit(AVD_Audio *audio, AVD_AudioStreamingPlayer *player, AVD_Size bufferCount);
void avdAudioStreamingPlayerShutdown(AVD_Audio *audio, AVD_AudioStreamingPlayer *player);
bool avdAudioStreamingPlayerAddChunk(AVD_Audio *audio, AVD_AudioStreamingPlayer *player, const void *data, AVD_Size dataSize);
bool avdAudioStreamingPlayerUpdate(AVD_Audio *audio, AVD_AudioStreamingPlayer *player);

#endif // AVD_AUDIO_STREAMING_PLAYER_H