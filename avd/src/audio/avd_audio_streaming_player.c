#include "audio/avd_audio_streaming_player.h"
#include "audio/avd_audio_core.h"
#include "core/avd_types.h"

bool avdAudioStreamingPlayerInit(AVD_Audio *audio, AVD_AudioStreamingPlayer *player, AVD_Size bufferCount)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(bufferCount > 0 && bufferCount <= AVD_AUDIO_STREAMING_PLAYER_MAX_BUFFERS);

    memset(player, 0, sizeof(AVD_AudioStreamingPlayer));
    player->bufferCount = bufferCount;

    player->initialized = true;
    return true;
}

void avdAudioStreamingPlayerShutdown(AVD_Audio *audio, AVD_AudioStreamingPlayer *player)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);

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

    return true;
}