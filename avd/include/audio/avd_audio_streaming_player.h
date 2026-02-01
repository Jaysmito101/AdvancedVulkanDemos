#ifndef AVD_AUDIO_STREAMING_PLAYER_H
#define AVD_AUDIO_STREAMING_PLAYER_H

#include "audio/avd_audio_clip.h"
#include "audio/avd_audio_core.h"
#include "core/avd_types.h"

#ifndef AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS
#define AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS 8
#endif

typedef enum {
    AVD_AUDIO_STREAMING_PLAYER_STATE_IDLE,
    AVD_AUDIO_STREAMING_PLAYER_STATE_READY,
    AVD_AUDIO_STREAMING_PLAYER_STATE_PLAYING,
    AVD_AUDIO_STREAMING_PLAYER_STATE_PAUSED
} AVD_AudioStreamingPlayerState;

typedef struct {
    AVD_Bool inUse;
    AVD_UInt32 lastUpdated;
    AVD_AudioClip clip;
} AVD_AudioStreamingPlayerBufferChunk;

typedef struct {
    bool initialized;
    AVD_Audio *audio;
    AVD_AudioStream stream;

    AVD_UInt32 sampleRate;
    AVD_UInt32 channels;
    AVD_UInt32 bitsPerSample;
    AVD_Bool formatInitialized;

    AVD_Size bufferCount;
    AVD_AudioStreamingPlayerBufferChunk chunks[AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS];
    AVD_Size chunkCount;

    AVD_AudioStreamingPlayerState state;

    AVD_Size currentChunkIndex;
    AVD_Size sampleOffsetInChunk;

    AVD_Size totalSamplesWritten;
    AVD_Size buffersProcessed;
} AVD_AudioStreamingPlayer;

bool avdAudioStreamingPlayerInit(AVD_Audio *audio, AVD_AudioStreamingPlayer *player, AVD_Size bufferCount);

void avdAudioStreamingPlayerShutdown(AVD_AudioStreamingPlayer *player);

bool avdAudioStreamingPlayerAddChunk(AVD_AudioStreamingPlayer *player, const void *data, AVD_Size dataSize);

bool avdAudioStreamingPlayerAddRawChunk(
    AVD_AudioStreamingPlayer *player,
    const void *data,
    AVD_Size dataSize,
    AVD_UInt32 sampleRate,
    AVD_UInt32 channels,
    AVD_UInt32 bitsPerSample);

bool avdAudioStreamingPlayerPlay(AVD_AudioStreamingPlayer *player);
bool avdAudioStreamingPlayerPause(AVD_AudioStreamingPlayer *player);
bool avdAudioStreamingPlayerResume(AVD_AudioStreamingPlayer *player);
bool avdAudioStreamingPlayerStop(AVD_AudioStreamingPlayer *player);

AVD_AudioStreamingPlayerState avdAudioStreamingPlayerGetState(AVD_AudioStreamingPlayer *player);
bool avdAudioStreamingPlayerIsPlaying(AVD_AudioStreamingPlayer *player);

AVD_Size avdAudioStreamingPlayerGetQueuedChunks(AVD_AudioStreamingPlayer *player);
AVD_Size avdAudioStreamingPlayerGetQueuedBytes(AVD_AudioStreamingPlayer *player);
bool avdAudioStreamingPlayerIsFed(AVD_AudioStreamingPlayer *player);

bool avdAudioStreamingPlayerGetBufferedDurationMs(AVD_AudioStreamingPlayer *player, AVD_Float *outDurationMs);
bool avdAudioStreamingPlayerGetTimePlayedMs(AVD_AudioStreamingPlayer *player, AVD_Float *outTimeMs);

bool avdAudioStreamingPlayerUpdate(AVD_AudioStreamingPlayer *player);

#endif // AVD_AUDIO_STREAMING_PLAYER_H