#include "audio/avd_audio_streaming_player.h"
#include "audio/avd_audio_clip.h"
#include "audio/avd_audio_core.h"
#include "core/avd_types.h"

static bool __avdAudioStreamingPlayerCallback(
    const void *inputBuffer,
    void *outputBuffer,
    AVD_Size framesPerBuffer,
    AVD_UInt32 statusFlags,
    void *userData)
{
    (void)inputBuffer;
    (void)statusFlags;

    AVD_AudioStreamingPlayer *player = (AVD_AudioStreamingPlayer *)userData;
    float *out                       = (float *)outputBuffer;

    if (!player || player->state != AVD_AUDIO_STREAMING_PLAYER_STATE_PLAYING) {
        AVD_Size channels = player ? player->channels : 2;
        AVD_Size size     = framesPerBuffer * channels;
        for (AVD_Size i = 0; i < size; i++) {
            out[i] = 0.0f;
        }
        return true;
    }

    AVD_Size channels       = player->channels;
    AVD_Size samplesNeeded  = framesPerBuffer * channels;
    AVD_Size samplesWritten = 0;

    while (samplesWritten < samplesNeeded) {
        AVD_Int32 currentChunkIdx = -1;

        if (player->currentChunkIndex < AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS) {
            AVD_AudioStreamingPlayerBufferChunk *current = &player->chunks[player->currentChunkIndex];
            if (current->inUse && current->clip.samples) {
                AVD_Size samplesInChunk = current->clip.sampleCount;
                if (player->sampleOffsetInChunk < samplesInChunk) {
                    currentChunkIdx = (AVD_Int32)player->currentChunkIndex;
                }
            }
        }

        if (currentChunkIdx < 0) {
            AVD_UInt32 earliestTime = UINT32_MAX;
            for (AVD_Size i = 0; i < player->bufferCount; i++) {
                AVD_AudioStreamingPlayerBufferChunk *chunk = &player->chunks[i];
                if (chunk->inUse && chunk->clip.samples && chunk->lastUpdated < earliestTime) {
                    currentChunkIdx = (AVD_Int32)i;
                    earliestTime    = chunk->lastUpdated;
                }
            }

            if (currentChunkIdx >= 0) {
                player->currentChunkIndex   = (AVD_Size)currentChunkIdx;
                player->sampleOffsetInChunk = 0;
            }
        }

        if (currentChunkIdx < 0) {
            for (AVD_Size i = samplesWritten; i < samplesNeeded; i++) {
                out[i] = 0.0f;
            }
            break;
        }

        AVD_AudioStreamingPlayerBufferChunk *chunk = &player->chunks[currentChunkIdx];
        AVD_AudioClip *clip                        = &chunk->clip;
        AVD_Size samplesInChunk                    = clip->sampleCount;
        AVD_Size samplesRemaining                  = samplesInChunk - player->sampleOffsetInChunk;
        AVD_Size samplesToRead                     = (samplesNeeded - samplesWritten < samplesRemaining) ? (samplesNeeded - samplesWritten) : samplesRemaining;

        for (AVD_Size i = 0; i < samplesToRead; i++) {
            AVD_Float sample = 0.0f;
            avdAudioClipSampleAtIndex(clip, player->sampleOffsetInChunk + i, &sample);
            out[samplesWritten + i] = sample;
        }

        player->sampleOffsetInChunk += samplesToRead;
        samplesWritten += samplesToRead;
        player->totalSamplesWritten += samplesToRead;

        if (player->sampleOffsetInChunk >= samplesInChunk) {
            chunk->inUse = false;
            player->chunkCount--;
            player->buffersProcessed++;
            player->currentChunkIndex   = AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS;
            player->sampleOffsetInChunk = 0;
        }
    }

    return true;
}

bool avdAudioStreamingPlayerInit(AVD_Audio *audio, AVD_AudioStreamingPlayer *player, AVD_Size bufferCount)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);

    memset(player, 0, sizeof(AVD_AudioStreamingPlayer));
    player->initialized       = true;
    player->audio             = audio;
    player->bufferCount       = bufferCount;
    player->currentChunkIndex = AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS;
    player->state             = AVD_AUDIO_STREAMING_PLAYER_STATE_IDLE;
    player->formatInitialized = false;

    return true;
}

void avdAudioStreamingPlayerShutdown(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    if (player->state == AVD_AUDIO_STREAMING_PLAYER_STATE_PLAYING ||
        player->state == AVD_AUDIO_STREAMING_PLAYER_STATE_PAUSED) {
        avdAudioStreamingPlayerStop(player);
    }

    if (player->stream.stream) {
        avdAudioStreamDestroy(player->audio, &player->stream);
    }

    for (AVD_Size i = 0; i < player->bufferCount; i++) {
        if (player->chunks[i].clip.samples) {
            avdAudioClipFree(&player->chunks[i].clip);
        }
    }

    memset(player, 0, sizeof(AVD_AudioStreamingPlayer));
}

static bool __avdAudioStreamingPlayerAddClip(AVD_AudioStreamingPlayer *player, AVD_AudioClip *clip)
{
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(clip != NULL);

    if (!player->initialized) {
        AVD_LOG_ERROR("Audio streaming player not initialized. Call avdAudioStreamingPlayerCreate or avdAudioStreamingPlayerInit first.");
        return false;
    }

    if (!player->formatInitialized) {
        player->sampleRate        = clip->sampleRate;
        player->channels          = clip->channels;
        player->bitsPerSample     = clip->bitsPerSample;
        player->formatInitialized = true;
        player->state             = AVD_AUDIO_STREAMING_PLAYER_STATE_READY;
        AVD_LOG_DEBUG("Auto-initialized streaming player: %u Hz, %u channels, %u bits", clip->sampleRate, clip->channels, clip->bitsPerSample);
    } else {
        if (player->sampleRate != clip->sampleRate ||
            player->channels != clip->channels ||
            player->bitsPerSample != clip->bitsPerSample) {
            AVD_LOG_ERROR("Chunk format mismatch! Expected: %u Hz, %u ch, %u bits. Got: %u Hz, %u ch, %u bits",
                          player->sampleRate, player->channels, player->bitsPerSample,
                          clip->sampleRate, clip->channels, clip->bitsPerSample);
            return false;
        }
    }

    // AVD_LOG_DEBUG("Adding audio chunk, current chunk count: %zu", player->chunkCount + 1);

    AVD_Int32 freeIndex = -1;
    AVD_Int32 lruIndex  = -1;
    AVD_UInt32 minTime  = UINT32_MAX;

    for (AVD_Size i = 0; i < player->bufferCount; i++) {
        if (!player->chunks[i].inUse) {
            freeIndex = (AVD_Int32)i;
            break;
        }
        if (player->chunks[i].lastUpdated < minTime) {
            minTime  = player->chunks[i].lastUpdated;
            lruIndex = (AVD_Int32)i;
        }
    }

    AVD_Int32 targetIndex = (freeIndex != -1) ? freeIndex : lruIndex;

    if (targetIndex != -1) {
        AVD_AudioStreamingPlayerBufferChunk *chunk = &player->chunks[targetIndex];

        if (chunk->clip.samples) {
            avdAudioClipFree(&chunk->clip);
        }
        if (!chunk->inUse) {
            player->chunkCount++;
        }

        chunk->inUse       = true;
        chunk->lastUpdated = (AVD_UInt32)clock();
        chunk->clip        = *clip;

        return true;
    }

    return false;
}

bool avdAudioStreamingPlayerAddChunk(AVD_AudioStreamingPlayer *player, const void *data, AVD_Size dataSize)
{
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(data != NULL);
    AVD_ASSERT(dataSize > 0);

    AVD_AudioClip clip = {0};
    if (!avdAudioClipFromMemory(data, dataSize, &clip)) {
        AVD_LOG_ERROR("Failed to decode audio chunk");
        return false;
    }

    bool result = __avdAudioStreamingPlayerAddClip(player, &clip);
    if (!result) {
        avdAudioClipFree(&clip);
    }

    return result;
}

bool avdAudioStreamingPlayerAddRawChunk(
    AVD_AudioStreamingPlayer *player,
    const void *data,
    AVD_Size dataSize,
    AVD_UInt32 sampleRate,
    AVD_UInt32 channels,
    AVD_UInt32 bitsPerSample)
{
    AVD_ASSERT(player != NULL);

    if (data == NULL || dataSize == 0) {
        return false;
    }

    AVD_AudioClip clip = {0};
    if (!avdAudioClipFromRawPCM(data, dataSize, sampleRate, channels, bitsPerSample, &clip)) {
        AVD_LOG_ERROR("Failed to create audio clip from raw PCM");
        return false;
    }

    bool result = __avdAudioStreamingPlayerAddClip(player, &clip);
    if (!result) {
        avdAudioClipFree(&clip);
    }

    return result;
}

bool avdAudioStreamingPlayerPlay(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(player->audio != NULL);

    if (player->state == AVD_AUDIO_STREAMING_PLAYER_STATE_PLAYING) {
        return true;
    }

    if (player->state == AVD_AUDIO_STREAMING_PLAYER_STATE_PAUSED) {
        return avdAudioStreamingPlayerResume(player);
    }

    if (!player->formatInitialized) {
        AVD_LOG_ERROR("Cannot play: format not initialized");
        return false;
    }

    if (!player->stream.stream) {
        bool success = avdAudioOpenOutputStream(
            player->audio,
            &player->stream,
            AVD_AUDIO_DEVICE_DEFAULT,
            player->sampleRate,
            player->channels == 2,
            256,
            __avdAudioStreamingPlayerCallback,
            player);

        if (!success) {
            AVD_LOG_ERROR("Failed to open audio output stream for streaming playback");
            return false;
        }
    }

    if (!avdAudioStartStream(player->audio, &player->stream)) {
        AVD_LOG_ERROR("Failed to start audio stream for streaming playback");
        avdAudioStreamDestroy(player->audio, &player->stream);
        return false;
    }

    player->state = AVD_AUDIO_STREAMING_PLAYER_STATE_PLAYING;
    return true;
}

bool avdAudioStreamingPlayerPause(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    if (player->state != AVD_AUDIO_STREAMING_PLAYER_STATE_PLAYING) {
        return false;
    }

    player->state = AVD_AUDIO_STREAMING_PLAYER_STATE_PAUSED;
    return true;
}

bool avdAudioStreamingPlayerResume(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    if (player->state != AVD_AUDIO_STREAMING_PLAYER_STATE_PAUSED) {
        return false;
    }

    player->state = AVD_AUDIO_STREAMING_PLAYER_STATE_PLAYING;
    return true;
}

bool avdAudioStreamingPlayerStop(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    if (player->stream.stream) {
        avdAudioAbortStream(player->audio, &player->stream);
        avdAudioStreamDestroy(player->audio, &player->stream);
        player->stream.stream = NULL;
    }

    for (AVD_Size i = 0; i < player->bufferCount; i++) {
        player->chunks[i].inUse = false;
    }

    player->chunkCount          = 0;
    player->state               = player->formatInitialized ? AVD_AUDIO_STREAMING_PLAYER_STATE_READY : AVD_AUDIO_STREAMING_PLAYER_STATE_IDLE;
    player->currentChunkIndex   = AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS;
    player->sampleOffsetInChunk = 0;
    player->totalSamplesWritten = 0;
    player->buffersProcessed    = 0;

    return true;
}

AVD_AudioStreamingPlayerState avdAudioStreamingPlayerGetState(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);
    return player->state;
}

bool avdAudioStreamingPlayerIsPlaying(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);
    return player->state == AVD_AUDIO_STREAMING_PLAYER_STATE_PLAYING;
}

bool avdAudioStreamingPlayerClearBuffers(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    for (AVD_Size i = 0; i < player->bufferCount; i++) {
        player->chunks[i].inUse = false;
    }

    player->chunkCount          = 0;
    player->currentChunkIndex   = AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS;
    player->sampleOffsetInChunk = 0;
    player->totalSamplesWritten = 0;
    player->buffersProcessed    = 0;

    return true;
}

AVD_Size avdAudioStreamingPlayerGetQueuedChunks(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);
    return player->chunkCount;
}

AVD_Size avdAudioStreamingPlayerGetQueuedBytes(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    AVD_Size totalBytes = 0;

    for (AVD_Size i = 0; i < player->bufferCount; i++) {
        if (player->chunks[i].inUse && player->chunks[i].clip.samples) {
            AVD_AudioClip *clip     = &player->chunks[i].clip;
            AVD_Size bytesPerSample = clip->bitsPerSample / 8;
            AVD_Size clipBytes      = clip->sampleCount * bytesPerSample;

            if (i == player->currentChunkIndex) {
                AVD_Size playedBytes = player->sampleOffsetInChunk * bytesPerSample;
                if (clipBytes > playedBytes) {
                    totalBytes += clipBytes - playedBytes;
                }
            } else {
                totalBytes += clipBytes;
            }
        }
    }

    return totalBytes;
}

bool avdAudioStreamingPlayerIsFed(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);
    return player->chunkCount > 0;
}

bool avdAudioStreamingPlayerGetBufferedDurationMs(AVD_AudioStreamingPlayer *player, AVD_Float *outDurationMs)
{
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(outDurationMs != NULL);

    if (!player->formatInitialized || player->sampleRate == 0 || player->channels == 0) {
        *outDurationMs = 0.0f;
        return false;
    }

    AVD_Float totalDurationMs = 0.0f;

    for (AVD_Size i = 0; i < player->bufferCount; i++) {
        if (player->chunks[i].inUse && player->chunks[i].clip.samples) {
            AVD_AudioClip *clip      = &player->chunks[i].clip;
            AVD_Float clipDurationMs = avdAudioClipDurationMs(clip);

            if (i == player->currentChunkIndex) {
                AVD_Size samplesInChunk  = clip->sampleCount;
                AVD_Float playedFraction = (AVD_Float)player->sampleOffsetInChunk / (AVD_Float)samplesInChunk;
                totalDurationMs += clipDurationMs * (1.0f - playedFraction);
            } else {
                totalDurationMs += clipDurationMs;
            }
        }
    }

    *outDurationMs = totalDurationMs;
    return true;
}

AVD_Float avdAudioStreamingPlayerGetTimePlayed(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    if (!player->formatInitialized || player->sampleRate == 0 || player->channels == 0) {
        return 0.0f;
    }

    AVD_Size totalFrames = player->totalSamplesWritten / player->channels;
    return ((AVD_Float)totalFrames / (AVD_Float)player->sampleRate);
}

AVD_Float avdAudioStreamingPlayerGetTimeOffsetInChunk(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    if (!player->formatInitialized || player->sampleRate == 0 || player->channels == 0) {
        return 0.0f;
    }

    if (player->currentChunkIndex >= AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS) {
        return 0.0f;
    }

    AVD_AudioStreamingPlayerBufferChunk *chunk = &player->chunks[player->currentChunkIndex];
    if (!chunk->inUse || !chunk->clip.samples) {
        return 0.0f;
    }

    AVD_Size samplesInChunk = chunk->clip.sampleCount;
    if (samplesInChunk == 0) {
        return 0.0f;
    }

    AVD_Size currentFrame = player->sampleOffsetInChunk / player->channels;

    return ((AVD_Float)currentFrame / (AVD_Float)player->sampleRate);
}

bool avdAudioStreamingPlayerUpdate(AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(player != NULL);

    for (AVD_Size i = 0; i < player->bufferCount; i++) {
        if (!player->chunks[i].inUse && player->chunks[i].clip.samples) {
            avdAudioClipFree(&player->chunks[i].clip);
        }
    }

    return true;
}