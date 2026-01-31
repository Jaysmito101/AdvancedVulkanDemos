#include "core/avd_base.h"
#include "scenes/hls_player/avd_scene_hls_player_segment_store.h"

bool avdHLSSegmentStoreInit(AVD_HLSSegmentStore *store)
{
    AVD_ASSERT(store);

    memset(store, 0, sizeof(AVD_HLSSegmentStore));

    return true;
}

void avdHLSSegmentStoreDestroy(AVD_HLSSegmentStore *store)
{
    AVD_ASSERT(store);

    for (AVD_Size i = 0; i < store->loadedSegmentCount; i++) {
        avdHLSSegmentAVDataFree(&store->loadedSegments[i]);
    }

    memset(store, 0, sizeof(AVD_HLSSegmentStore));
}

bool avdHLSSegmentStoreClear(AVD_HLSSegmentStore *store)
{
    AVD_ASSERT(store);

    for (AVD_Size i = 0; i < store->loadedSegmentCount; i++) {
        avdHLSSegmentAVDataFree(&store->loadedSegments[i]);
    }

    store->loadedSegmentCount = 0;
    return true;
}

bool avdHLSSegmentStoreHasSegment(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId)
{
    AVD_ASSERT(store != NULL);

    for (AVD_Size i = 0; i < store->loadedSegmentCount; i++) {
        if (store->loadedSegments[i].source == sourceIndex &&
            store->loadedSegments[i].segmentId == segmentId) {
            return true;
        }
    }

    return false;
}

bool avdHLSSegmentStoreAdd(AVD_HLSSegmentStore *store, AVD_HLSSegmentAVData data)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(data.segmentId != 0); // 0 means uninitialized

    AVD_Size freeSegmentIndex   = AVD_SCENE_HLS_PLAYER_MAX_LOADED_SEGMENTS;
    AVD_Size oldestSegmentIndex = 0;

    AVD_UInt32 oldestAccessTime = UINT32_MAX;
    for (AVD_Size i = 0; i < store->loadedSegmentCount; i++) {
        if (store->loadedSegments[i].source == data.source &&
            store->loadedSegments[i].segmentId == data.segmentId) {
            AVD_LOG_WARN("HLS Segment Store: segment %u for source %u already exists in store", data.segmentId, data.source);
            return false;
        }

        if (store->loadedSegments[i].segmentId == 0) {
            freeSegmentIndex = i;
            break;
        }

        if (store->segmentAccessTime[i] < oldestAccessTime) {
            oldestAccessTime   = store->segmentAccessTime[i];
            oldestSegmentIndex = i;
        }
    }

    if (store->loadedSegmentCount < AVD_SCENE_HLS_PLAYER_MAX_LOADED_SEGMENTS) {
        freeSegmentIndex = store->loadedSegmentCount;
        store->loadedSegmentCount++;
    } else {
        freeSegmentIndex = oldestSegmentIndex;
        avdHLSSegmentAVDataFree(&store->loadedSegments[freeSegmentIndex]);
    }

    store->loadedSegments[freeSegmentIndex]    = data;
    store->segmentAccessTime[freeSegmentIndex] = (AVD_UInt32)time(NULL);

    return true;
}

bool avdHLSSegmentStoreAcquire(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId, AVD_HLSSegmentAVData *data)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(data != NULL);

    for (AVD_Size i = 0; i < store->loadedSegmentCount; i++) {
        if (store->loadedSegments[i].source == sourceIndex &&
            store->loadedSegments[i].segmentId == segmentId) {
            *data                       = store->loadedSegments[i];
            store->segmentAccessTime[i] = (AVD_UInt32)time(NULL);
            memset(&store->loadedSegments[i], 0, sizeof(AVD_HLSSegmentAVData));
            return true;
        }
    }

    return false;
}

bool avdHLSSegmentStoreFindNextSegment(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 currentSegmentId, AVD_Size *outSegmentId)
{
    AVD_ASSERT(store != NULL);

    AVD_UInt32 nextSegmentId = UINT32_MAX;
    for (AVD_Size i = 0; i < store->loadedSegmentCount; i++) {
        if (store->loadedSegments[i].source == sourceIndex &&
            store->loadedSegments[i].segmentId > currentSegmentId &&
            store->loadedSegments[i].segmentId < nextSegmentId) {
            nextSegmentId = store->loadedSegments[i].segmentId;
        }
    }
    if (nextSegmentId == UINT32_MAX) {
        return false;
    }
    *outSegmentId = nextSegmentId;
    return true;
}

void avdHLSSegmentAVDataFree(AVD_HLSSegmentAVData *avData)
{
    AVD_ASSERT(avData != NULL);

    if (avData->h264Buffer) {
        AVD_FREE(avData->h264Buffer);
        avData->h264Buffer = NULL;
        avData->h264Size   = 0;
    }

    if (avData->aacBuffer) {
        AVD_FREE(avData->aacBuffer);
        avData->aacBuffer = NULL;
        avData->aacSize   = 0;
    }

    memset(avData, 0, sizeof(AVD_HLSSegmentAVData));
}