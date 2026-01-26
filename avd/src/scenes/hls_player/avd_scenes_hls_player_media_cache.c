#include "scenes/hls_player/avd_hls_media_cache.h"

#include "core/avd_base.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

bool avdHLSMediaCacheInit(AVD_HLSMediaCache *cache)
{
    AVD_ASSERT(cache != NULL);

    cache->mutex = picoThreadMutexCreate();
    AVD_CHECK_MSG(cache->mutex != NULL, "Failed to create media cache mutex");
    memset(cache->entries, 0, sizeof(AVD_HLSMediaCacheEntry) * AVD_HLS_MEDIA_CACHE_SIZE);

    return true;
}

void avdHLSMediaCacheDestroy(AVD_HLSMediaCache *cache)
{
    AVD_ASSERT(cache != NULL);

    if (cache->mutex) {
        picoThreadMutexLock(cache->mutex, PICO_THREAD_INFINITE);
        for (AVD_Size i = 0; i < AVD_HLS_MEDIA_CACHE_SIZE; i++) {
            AVD_HLSMediaCacheEntry *entry = &cache->entries[i];
            if (entry->data) {
                AVD_FREE(entry->data);
                entry->data     = NULL;
                entry->dataSize = 0;
            }
        }
        picoThreadMutexUnlock(cache->mutex);
        picoThreadMutexDestroy(cache->mutex);
        cache->mutex = NULL;
    }
}

bool avdHLSMediaCacheQuery(AVD_HLSMediaCache *cache, AVD_UInt32 key, char **data, AVD_Size *dataSize)
{
    AVD_ASSERT(cache != NULL);
    AVD_ASSERT(data != NULL);
    AVD_ASSERT(dataSize != NULL);

    picoThreadMutexLock(cache->mutex, PICO_THREAD_INFINITE);

    for (AVD_Size i = 0; i < AVD_HLS_MEDIA_CACHE_SIZE; i++) {
        AVD_HLSMediaCacheEntry *entry = &cache->entries[i];
        if (entry->key == key) {
            *data = AVD_MALLOC(entry->dataSize);
            AVD_CHECK_MSG(*data != NULL, "Failed to allocate memory for media cache entry data");
            memcpy(*data, entry->data, entry->dataSize);
            *dataSize        = entry->dataSize;
            entry->timestamp = (AVD_UInt32)time(NULL);
            picoThreadMutexUnlock(cache->mutex);
            return true;
        }
    }

    picoThreadMutexUnlock(cache->mutex);
    return false;
}

bool avdHLSMediaCacheInsert(AVD_HLSMediaCache *cache, AVD_UInt32 key, const char *data, AVD_Size dataSize)
{
    AVD_ASSERT(cache != NULL);
    AVD_ASSERT(data != NULL);
    AVD_ASSERT(dataSize > 0);

    picoThreadMutexLock(cache->mutex, PICO_THREAD_INFINITE);

    AVD_Size lruIndex       = 0;
    AVD_UInt32 lruTimestamp = UINT32_MAX;

    for (AVD_Size i = 0; i < AVD_HLS_MEDIA_CACHE_SIZE; i++) {
        AVD_HLSMediaCacheEntry *entry = &cache->entries[i];
        if (entry->timestamp < lruTimestamp) {
            lruTimestamp = entry->timestamp;
            lruIndex     = i;
        }
        if (entry->key == 0) {
            break;
        }
    }

    AVD_HLSMediaCacheEntry *entry = &cache->entries[lruIndex];

    if (entry->capacity < dataSize) {
        if (entry->data) {
            AVD_FREE(entry->data);
        }
        entry->data = (char *)AVD_MALLOC(dataSize);
        AVD_CHECK_MSG(entry->data != NULL, "Failed to allocate memory for media cache entry data");
        entry->capacity = dataSize;
    }
    memcpy(entry->data, data, dataSize);
    entry->dataSize  = dataSize;
    entry->key       = key;
    entry->timestamp = (AVD_UInt32)time(NULL);

    picoThreadMutexUnlock(cache->mutex);
    return true;
}
