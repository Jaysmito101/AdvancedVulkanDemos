
#ifndef AVD_SCENES_HLS_PLAYER_SEGMENT_STORE_H
#define AVD_SCENES_HLS_PLAYER_SEGMENT_STORE_H

#include "core/avd_core.h"
#include "core/avd_types.h"

#define AVD_SCENE_HLS_PLAYER_MAX_LOADED_SEGMENTS 8 * 6

typedef struct {
    AVD_Size source;
    AVD_Size segmentId;

    AVD_Float duration;

    uint8_t *h264Buffer;
    AVD_Size h264Size;

    uint8_t *aacBuffer;
    AVD_Size aacSize;
} AVD_HLSSegmentAVData;

typedef struct {
    AVD_HLSSegmentAVData loadedSegments[AVD_SCENE_HLS_PLAYER_MAX_LOADED_SEGMENTS];
    AVD_UInt32 segmentAccessTime[AVD_SCENE_HLS_PLAYER_MAX_LOADED_SEGMENTS];
    AVD_Size loadedSegmentCount;
} AVD_HLSSegmentStore;

bool avdHLSSegmentStoreInit(AVD_HLSSegmentStore *store);
void avdHLSSegmentStoreDestroy(AVD_HLSSegmentStore *store);
bool avdHLSSegmentStoreClear(AVD_HLSSegmentStore *store);

bool avdHLSSegmentStoreHasSegment(AVD_HLSSegmentStore *store, AVD_Size sourceIndex, AVD_Size segmentId);
bool avdHLSSegmentStoreAdd(AVD_HLSSegmentStore *store, AVD_HLSSegmentAVData data);
bool avdHLSSegmentStoreAcquire(AVD_HLSSegmentStore *store, AVD_Size sourceIndex, AVD_Size segmentId, AVD_HLSSegmentAVData *data);
bool avdHLSSegmentStoreFindNextSegment(AVD_HLSSegmentStore *store, AVD_Size sourceIndex, AVD_Size currentSegmentId, AVD_Size *outSegmentId);

void avdHLSSegmentAVDataFree(AVD_HLSSegmentAVData *avData);

#endif // AVD_SCENES_HLS_PLAYER_SEGMENT_STORE_H