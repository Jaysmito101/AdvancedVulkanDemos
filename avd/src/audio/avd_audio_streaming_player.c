#include "audio/avd_audio_streaming_player.h"
#include "al.h"
#include "audio/avd_audio_core.h"
#include "core/avd_types.h"

bool avdAudioStreamingPlayerInit(AVD_Audio *audio, AVD_AudioStreamingPlayer *player, AVD_Size bufferCount)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(bufferCount > 0 && bufferCount <= AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS);

    memset(player, 0, sizeof(AVD_AudioStreamingPlayer));
    player->bufferCount = bufferCount;

    for (AVD_Size i = 0; i < bufferCount; i++) {
        if (!avdAudioBufferCreate(audio, &player->buffers[i])) {
            AVD_LOG_ERROR("Failed to create audio buffer for streaming player");
            avdAudioStreamingPlayerShutdown(audio, player);
            return false;
        }

        // if (!avdAudioLoadEmptyBuffer(audio, player->buffers[i], 44100, 2)) {
        //     AVD_LOG_ERROR("Failed to load empty buffer for streaming player");
        //     avdAudioStreamingPlayerShutdown(audio, player);
        //     return false;
        // }

        if (!avdAudioLoadBufferFromFile(audio, "./hls_segments/source_0/aac_adts/393724.aac", player->buffers[i])) {
            AVD_LOG_ERROR("Failed to load empty buffer for streaming player");
            avdAudioStreamingPlayerShutdown(audio, player);
            return false;
        }
    }

    if (!avdAudioSourceCreate(audio, &player->source)) {
        AVD_LOG_ERROR("Failed to create audio source for streaming player");
        avdAudioStreamingPlayerShutdown(audio, player);
        return false;
    }

    if (!avdAudioSourceUnlinkBuffer(audio, player->source)) {
        AVD_LOG_ERROR("Failed to unlink buffers from streaming player source");
        avdAudioStreamingPlayerShutdown(audio, player);
        return false;
    }

    if (!avdAudioSourceEnqueueBuffers(audio, player->source, player->buffers, bufferCount)) {
        AVD_LOG_ERROR("Failed to enqueue buffers for streaming player");
        avdAudioStreamingPlayerShutdown(audio, player);
        return false;
    }

    if (!avdAudioSourcePlay(audio, player->source)) {
        AVD_LOG_ERROR("Failed to start audio source playback for streaming player");
        avdAudioStreamingPlayerShutdown(audio, player);
        return false;
    }

    player->initialized = true;
    return true;
}

void avdAudioStreamingPlayerShutdown(AVD_Audio *audio, AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);

    if (player->source) {
        avdAudioSourceStop(audio, player->source);
        avdAudioSourceDestroy(audio, player->source);
    }

    for (AVD_Size i = 0; i < player->bufferCount; i++) {
        if (player->buffers[i]) {
            avdAudioBufferDestroy(audio, player->buffers[i]);
        }
    }
    memset(player, 0, sizeof(AVD_AudioStreamingPlayer));
}

bool avdAudioStreamingPlayerAddChunk(AVD_Audio *audio, AVD_AudioStreamingPlayer *player, const void *data, AVD_Size dataSize)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);

    if (data == NULL || dataSize == 0) {
        return false;
    }

    if (dataSize > AVD_AUDIO_STREAMING_PLAYER_MAX_CHUNK_SIZE) {
        AVD_LOG_ERROR("Chunk size too large: %zu (max %d)", dataSize, AVD_AUDIO_STREAMING_PLAYER_MAX_CHUNK_SIZE);
        return false;
    }

    AVD_LOG_DEBUG("Adding audio chunk, current chunk count: %zu", player->chunkCount + 1);

    AVD_Int32 freeIndex = -1;
    AVD_Int32 lruIndex  = -1;
    AVD_UInt32 minTime  = UINT32_MAX;

    for (AVD_Size i = 0; i < AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS; i++) {
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

        if (!chunk->inUse) {
            player->chunkCount++;
        }

        chunk->inUse       = true;
        chunk->lastUpdated = (AVD_UInt32)clock();
        chunk->dataSize    = dataSize;
        memcpy(chunk->data, data, dataSize);

        return true;
    }

    return false;
}

bool avdAudioStreamingPlayerUpdate(AVD_Audio *audio, AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);

    AVD_Size processedCount = 0, queuedCount = 0;
    AVD_CHECK(avdAudioSourceGetProcessedBufferCount(audio, player->source, &processedCount));
    AVD_CHECK(avdAudioSourceGetQueuedBufferCount(audio, player->source, &queuedCount));

    while (processedCount > 0) {
        AVD_LOG_DEBUG("processed buffers: %zu, queued buffers: %zu", processedCount, queuedCount);

        AVD_AudioBuffer buffer = 0;
        AVD_CHECK(avdAudioSourceUnqueueBuffers(audio, player->source, &buffer, 1));

        AVD_Int32 chunkIndex = -1;
        // find the oldest in-use chunk
        AVD_UInt32 minTime = UINT32_MAX;
        for (AVD_Size i = 0; i < AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS; i++) {
            if (player->chunks[i].inUse && player->chunks[i].lastUpdated < minTime) {
                minTime    = player->chunks[i].lastUpdated;
                chunkIndex = (AVD_Int32)i;
            }
        }

        if (chunkIndex != -1) {
            AVD_AudioStreamingPlayerBufferChunk *chunk = &player->chunks[chunkIndex];

            AVD_CHECK(avdAudioLoadBufferFromMemory(
                audio,
                chunk->data,
                chunk->dataSize,
                buffer));

            chunk->inUse = false;
            player->chunkCount--;

            AVD_LOG_DEBUG("Streaming audio chunk, current chunk count: %zu", player->chunkCount);
        } else {
            AVD_LOG_WARN("No available audio chunk to stream, loading silent buffer, current chunk count: %zu", player->chunkCount);
            AVD_CHECK(avdAudioLoadEmptyBuffer(audio, buffer, 44100, 2));
        }

        AVD_CHECK(avdAudioSourceEnqueueBuffers(audio, player->source, &buffer, 1));

        processedCount--;
        player->buffersProcessed++;
    }

    return true;
}