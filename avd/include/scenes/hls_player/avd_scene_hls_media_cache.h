#ifndef AVD_HLS_MEDIA_CACHE_H
#define AVD_HLS_MEDIA_CACHE_H

#include "core/avd_types.h"
#include "pico/picoThreads.h"

#define AVD_HLS_MEDIA_CACHE_SIZE 16

typedef struct {
    char *data;
    AVD_Size dataSize;
    AVD_Size capacity;
    AVD_UInt32 key;
    AVD_UInt32 timestamp;
} AVD_HLSMediaCacheEntry;

typedef struct {
    AVD_HLSMediaCacheEntry entries[AVD_HLS_MEDIA_CACHE_SIZE];
    picoThreadMutex mutex;
} AVD_HLSMediaCache;

bool avdHLSMediaCacheInit(AVD_HLSMediaCache *cache);
void avdHLSMediaCacheDestroy(AVD_HLSMediaCache *cache);

bool avdHLSMediaCacheQuery(AVD_HLSMediaCache *cache, AVD_UInt32 key, char **data, AVD_Size *dataSize);
bool avdHLSMediaCacheInsert(AVD_HLSMediaCache *cache, AVD_UInt32 key, const char *data, AVD_Size dataSize);

#endif // AVD_HLS_MEDIA_CACHE_H
