#ifndef AVD_HLS_SEGMENT_STORE_H
#define AVD_HLS_SEGMENT_STORE_H

#include "core/avd_types.h"
#include "pico/picoThreads.h"
#include "vulkan/avd_vulkan_video.h"

#define AVD_HLS_SEGMENT_RING_SIZE    8
#define AVD_HLS_SEGMENT_MAX_SOURCES  4

typedef enum {
    AVD_HLS_SEGMENT_STATE_EMPTY = 0,
    AVD_HLS_SEGMENT_STATE_RESERVED,
    AVD_HLS_SEGMENT_STATE_READY,
    AVD_HLS_SEGMENT_STATE_PLAYING
} AVD_HLSSegmentState;

typedef struct {
    AVD_UInt32 segmentId;
    AVD_Float duration;
    AVD_HLSSegmentState state;
    AVD_H264Video *video;
} AVD_HLSSegmentSlot;

typedef struct {
    AVD_HLSSegmentSlot slots[AVD_HLS_SEGMENT_RING_SIZE];
    AVD_UInt32 minSegmentId;
    picoThreadMutex mutex;
} AVD_HLSSegmentRing;

typedef struct {
    AVD_HLSSegmentRing rings[AVD_HLS_SEGMENT_MAX_SOURCES];
    AVD_UInt32 sourceCount;
} AVD_HLSSegmentStore;

bool avdHLSSegmentStoreInit(AVD_HLSSegmentStore *store, AVD_UInt32 sourceCount);
void avdHLSSegmentStoreDestroy(AVD_HLSSegmentStore *store);
void avdHLSSegmentStoreClear(AVD_HLSSegmentStore *store);

bool avdHLSSegmentStoreReserve(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId);
bool avdHLSSegmentStoreCommit(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId, AVD_H264Video *video, AVD_Float duration);
AVD_H264Video *avdHLSSegmentStoreAcquire(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId);
void avdHLSSegmentStoreRelease(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId);
void avdHLSSegmentStoreAdvance(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 newMinSegmentId);
AVD_HLSSegmentSlot *avdHLSSegmentStoreGetSlot(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId);

bool avdHLSSegmentStoreHasSegment(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId);
bool avdHLSSegmentStoreFindNextSegment(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 currentSegmentId, AVD_UInt32 *outNextSegmentId);
AVD_UInt32 avdHLSSegmentStoreGetActiveSourcesBitfield(AVD_HLSSegmentStore *store, AVD_UInt32 *currentSegmentIds);

#endif // AVD_HLS_SEGMENT_STORE_H
