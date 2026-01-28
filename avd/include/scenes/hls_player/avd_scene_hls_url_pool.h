#ifndef AVD_HLS_URL_POOL_H
#define AVD_HLS_URL_POOL_H

#include "core/avd_types.h"
#include "pico/picoThreads.h"

#define AVD_HLS_URL_POOL_CAPACITY 256
#define AVD_HLS_URL_MAX_LENGTH   1024

typedef struct {
    char urls[AVD_HLS_URL_POOL_CAPACITY][AVD_HLS_URL_MAX_LENGTH];
    AVD_UInt32 hashes[AVD_HLS_URL_POOL_CAPACITY];
    AVD_UInt64 lastAccess[AVD_HLS_URL_POOL_CAPACITY];
    AVD_UInt32 count;
    AVD_UInt64 accessCounter;
    picoThreadMutex mutex;
} AVD_HLSURLPool;

bool avdHLSURLPoolInit(AVD_HLSURLPool *pool);
void avdHLSURLPoolDestroy(AVD_HLSURLPool *pool);

AVD_Bool avdHLSURLPoolInsert(AVD_HLSURLPool *pool, const char *url, AVD_UInt32* outHash);
const char *avdHLSURLPoolGet(AVD_HLSURLPool *pool, AVD_UInt32 hash);

void avdHLSURLPoolClear(AVD_HLSURLPool *pool);

#endif // AVD_HLS_URL_POOL_H
