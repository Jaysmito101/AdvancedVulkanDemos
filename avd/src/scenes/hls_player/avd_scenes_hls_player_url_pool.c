#include "scenes/hls_player/avd_scene_hls_url_pool.h"

#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"

#include <stdlib.h>
#include <string.h>

bool avdHLSURLPoolInit(AVD_HLSURLPool *pool)
{
    AVD_ASSERT(pool != NULL);

    memset(pool, 0, sizeof(AVD_HLSURLPool));
    pool->mutex = picoThreadMutexCreate();
    AVD_CHECK_MSG(pool->mutex != NULL, "Failed to create URL pool mutex");
    pool->count         = 0;
    pool->accessCounter = 0;

    return true;
}

void avdHLSURLPoolDestroy(AVD_HLSURLPool *pool)
{
    AVD_ASSERT(pool != NULL);

    if (pool->mutex) {
        picoThreadMutexDestroy(pool->mutex);
        pool->mutex = NULL;
    }
}

AVD_Bool avdHLSURLPoolInsert(AVD_HLSURLPool *pool, const char *url, AVD_UInt32 *outHash)
{
    AVD_ASSERT(pool != NULL);
    AVD_ASSERT(url != NULL);

    AVD_UInt32 hash = avdHashString(url);
    if (outHash) {
        *outHash = hash;
    }

    picoThreadMutexLock(pool->mutex, PICO_THREAD_INFINITE);

    for (AVD_UInt32 i = 0; i < pool->count; i++) {
        if (pool->hashes[i] == hash) {
            pool->lastAccess[i] = ++pool->accessCounter;
            picoThreadMutexUnlock(pool->mutex);
            return true;
        }
    }

    AVD_Size urlLen = strlen(url);
    if (urlLen >= AVD_HLS_URL_MAX_LENGTH) {
        picoThreadMutexUnlock(pool->mutex);
        return false;
    }

    AVD_UInt32 insertIdx = pool->count;
    if (pool->count >= AVD_HLS_URL_POOL_CAPACITY) {
        AVD_UInt32 lruIdx  = 0;
        AVD_UInt64 lruTime = pool->lastAccess[0];
        for (AVD_UInt32 i = 1; i < pool->count; i++) {
            if (pool->lastAccess[i] < lruTime) {
                lruTime = pool->lastAccess[i];
                lruIdx  = i;
            }
        }
        insertIdx = lruIdx;
    } else {
        pool->count++;
    }

    memcpy(pool->urls[insertIdx], url, urlLen + 1);
    pool->hashes[insertIdx]     = hash;
    pool->lastAccess[insertIdx] = ++pool->accessCounter;

    picoThreadMutexUnlock(pool->mutex);
    return true;
}

const char *avdHLSURLPoolGet(AVD_HLSURLPool *pool, AVD_UInt32 hash)
{
    AVD_ASSERT(pool != NULL);

    picoThreadMutexLock(pool->mutex, PICO_THREAD_INFINITE);

    for (AVD_UInt32 i = 0; i < pool->count; i++) {
        if (pool->hashes[i] == hash) {
            pool->lastAccess[i] = ++pool->accessCounter;
            const char *url     = pool->urls[i];
            picoThreadMutexUnlock(pool->mutex);
            return url;
        }
    }

    picoThreadMutexUnlock(pool->mutex);
    return NULL;
}

void avdHLSURLPoolClear(AVD_HLSURLPool *pool)
{
    AVD_ASSERT(pool != NULL);

    picoThreadMutexLock(pool->mutex, PICO_THREAD_INFINITE);
    memset(pool->urls, 0, sizeof(pool->urls));
    memset(pool->hashes, 0, sizeof(pool->hashes));
    memset(pool->lastAccess, 0, sizeof(pool->lastAccess));
    pool->count         = 0;
    pool->accessCounter = 0;
    picoThreadMutexUnlock(pool->mutex);
}
