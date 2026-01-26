#include "scenes/hls_player/avd_scene_hls_segment_store.h"

#include "core/avd_base.h"

#include <string.h>

static void __avdHLSSegmentStoreResetSlot(AVD_HLSSegmentSlot *slot)
{
    if (slot->h264Buffer) {
        AVD_FREE(slot->h264Buffer);
        slot->h264Buffer = NULL;
        slot->h264Size   = 0;
    }
    slot->segmentId = 0;
    slot->duration  = 0.0f;
    slot->state     = AVD_HLS_SEGMENT_STATE_EMPTY;
}

bool avdHLSSegmentStoreInit(AVD_HLSSegmentStore *store, AVD_UInt32 sourceCount)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceCount <= AVD_HLS_SEGMENT_MAX_SOURCES);

    memset(store, 0, sizeof(AVD_HLSSegmentStore));
    store->sourceCount = sourceCount;

    for (AVD_UInt32 i = 0; i < sourceCount; i++) {
        AVD_HLSSegmentRing *ring = &store->rings[i];
        ring->mutex              = picoThreadMutexCreate();
        AVD_CHECK_MSG(ring->mutex != NULL, "Failed to create segment ring mutex");
        ring->minSegmentId = 0;
        memset(ring->slots, 0, sizeof(AVD_HLSSegmentSlot) * AVD_HLS_SEGMENT_RING_SIZE);
    }

    return true;
}

void avdHLSSegmentStoreDestroy(AVD_HLSSegmentStore *store)
{
    AVD_ASSERT(store != NULL);

    for (AVD_UInt32 i = 0; i < store->sourceCount; i++) {
        AVD_HLSSegmentRing *ring = &store->rings[i];
        if (ring->mutex) {
            picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);
            for (AVD_UInt32 j = 0; j < AVD_HLS_SEGMENT_RING_SIZE; j++) {
                __avdHLSSegmentStoreResetSlot(&ring->slots[j]);
            }
            picoThreadMutexUnlock(ring->mutex);
            picoThreadMutexDestroy(ring->mutex);
            ring->mutex = NULL;
        }
    }

    store->sourceCount = 0;
}

void avdHLSSegmentStoreClear(AVD_HLSSegmentStore *store)
{
    AVD_ASSERT(store != NULL);

    for (AVD_UInt32 i = 0; i < store->sourceCount; i++) {
        AVD_HLSSegmentRing *ring = &store->rings[i];
        if (ring->mutex) {
            picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);
            for (AVD_UInt32 j = 0; j < AVD_HLS_SEGMENT_RING_SIZE; j++) {
                __avdHLSSegmentStoreResetSlot(&ring->slots[j]);
            }
            ring->minSegmentId = 0;
            picoThreadMutexUnlock(ring->mutex);
        }
    }
}

bool avdHLSSegmentStoreReserve(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceIndex < store->sourceCount);

    AVD_HLSSegmentRing *ring = &store->rings[sourceIndex];
    AVD_UInt32 slotIndex     = segmentId % AVD_HLS_SEGMENT_RING_SIZE;

    picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

    if (segmentId < ring->minSegmentId) {
        picoThreadMutexUnlock(ring->mutex);
        return false;
    }

    AVD_HLSSegmentSlot *slot = &ring->slots[slotIndex];
    if (slot->state != AVD_HLS_SEGMENT_STATE_EMPTY) {
        if (slot->segmentId == segmentId) {
            picoThreadMutexUnlock(ring->mutex);
            return false;
        }
        __avdHLSSegmentStoreResetSlot(slot);
    }

    slot->segmentId  = segmentId;
    slot->state      = AVD_HLS_SEGMENT_STATE_RESERVED;
    slot->h264Buffer = NULL;
    slot->h264Size   = 0;
    slot->duration   = 0.0f;

    picoThreadMutexUnlock(ring->mutex);
    return true;
}

bool avdHLSSegmentStoreCommit(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId, uint8_t *h264Buffer, AVD_Size h264Size, AVD_Float duration)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceIndex < store->sourceCount);

    AVD_HLSSegmentRing *ring = &store->rings[sourceIndex];
    AVD_UInt32 slotIndex     = segmentId % AVD_HLS_SEGMENT_RING_SIZE;

    picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

    AVD_HLSSegmentSlot *slot = &ring->slots[slotIndex];

    if (slot->segmentId != segmentId || slot->state != AVD_HLS_SEGMENT_STATE_RESERVED) {
        if (h264Buffer) {
            AVD_FREE(h264Buffer);
        }
        picoThreadMutexUnlock(ring->mutex);
        return false;
    }

    slot->h264Buffer = h264Buffer;
    slot->h264Size   = h264Size;
    slot->duration   = duration;
    slot->state      = AVD_HLS_SEGMENT_STATE_READY;

    picoThreadMutexUnlock(ring->mutex);
    return true;
}

uint8_t *avdHLSSegmentStoreAcquire(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId, AVD_Size *outH264Size)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceIndex < store->sourceCount);

    AVD_HLSSegmentRing *ring = &store->rings[sourceIndex];
    AVD_UInt32 slotIndex     = segmentId % AVD_HLS_SEGMENT_RING_SIZE;

    picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

    AVD_HLSSegmentSlot *slot = &ring->slots[slotIndex];

    if (slot->segmentId != segmentId || slot->state != AVD_HLS_SEGMENT_STATE_READY) {
        picoThreadMutexUnlock(ring->mutex);
        return NULL;
    }

    slot->state = AVD_HLS_SEGMENT_STATE_PLAYING;

    uint8_t *h264Buffer = slot->h264Buffer;
    if (outH264Size) {
        *outH264Size = slot->h264Size;
    }

    picoThreadMutexUnlock(ring->mutex);
    return h264Buffer;
}

AVD_HLSSegmentSlot *avdHLSSegmentStoreGetSlot(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceIndex < store->sourceCount);

    AVD_HLSSegmentRing *ring = &store->rings[sourceIndex];
    AVD_UInt32 slotIndex     = segmentId % AVD_HLS_SEGMENT_RING_SIZE;

    picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

    AVD_HLSSegmentSlot *slot = &ring->slots[slotIndex];

    if (slot->segmentId != segmentId) {
        picoThreadMutexUnlock(ring->mutex);
        return NULL;
    }

    picoThreadMutexUnlock(ring->mutex);
    return slot;
}

void avdHLSSegmentStoreRelease(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceIndex < store->sourceCount);

    AVD_HLSSegmentRing *ring = &store->rings[sourceIndex];
    AVD_UInt32 slotIndex     = segmentId % AVD_HLS_SEGMENT_RING_SIZE;

    picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

    AVD_HLSSegmentSlot *slot = &ring->slots[slotIndex];
    __avdHLSSegmentStoreResetSlot(slot);
    picoThreadMutexUnlock(ring->mutex);
}

void avdHLSSegmentStoreAdvance(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 newMinSegmentId)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceIndex < store->sourceCount);

    AVD_HLSSegmentRing *ring = &store->rings[sourceIndex];

    picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

    if (newMinSegmentId <= ring->minSegmentId) {
        picoThreadMutexUnlock(ring->mutex);
        return;
    }

    for (AVD_UInt32 i = 0; i < AVD_HLS_SEGMENT_RING_SIZE; i++) {
        AVD_HLSSegmentSlot *slot = &ring->slots[i];
        if (slot->state != AVD_HLS_SEGMENT_STATE_EMPTY && slot->segmentId < newMinSegmentId) {
            __avdHLSSegmentStoreResetSlot(slot);
        }
    }

    ring->minSegmentId = newMinSegmentId;

    picoThreadMutexUnlock(ring->mutex);
}

bool avdHLSSegmentStoreHasSegment(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 segmentId)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceIndex < store->sourceCount);

    AVD_HLSSegmentRing *ring = &store->rings[sourceIndex];
    AVD_UInt32 slotIndex     = segmentId % AVD_HLS_SEGMENT_RING_SIZE;

    picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

    AVD_HLSSegmentSlot *slot = &ring->slots[slotIndex];
    bool hasSegment          = (slot->segmentId == segmentId) && (slot->state == AVD_HLS_SEGMENT_STATE_READY || slot->state == AVD_HLS_SEGMENT_STATE_PLAYING);

    picoThreadMutexUnlock(ring->mutex);
    return hasSegment;
}

bool avdHLSSegmentStoreFindNextSegment(AVD_HLSSegmentStore *store, AVD_UInt32 sourceIndex, AVD_UInt32 currentSegmentId, AVD_UInt32 *outNextSegmentId)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(sourceIndex < store->sourceCount);
    AVD_ASSERT(outNextSegmentId != NULL);

    AVD_HLSSegmentRing *ring = &store->rings[sourceIndex];

    picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

    AVD_UInt32 nextSegmentId = UINT32_MAX;
    for (AVD_UInt32 i = 0; i < AVD_HLS_SEGMENT_RING_SIZE; i++) {
        AVD_HLSSegmentSlot *slot = &ring->slots[i];
        if ((slot->state == AVD_HLS_SEGMENT_STATE_READY || slot->state == AVD_HLS_SEGMENT_STATE_PLAYING) && slot->segmentId > currentSegmentId && slot->segmentId < nextSegmentId) {
            nextSegmentId = slot->segmentId;
        }
    }

    picoThreadMutexUnlock(ring->mutex);

    if (nextSegmentId == UINT32_MAX) {
        return false;
    }

    *outNextSegmentId = nextSegmentId;
    return true;
}

AVD_UInt32 avdHLSSegmentStoreGetActiveSourcesBitfield(AVD_HLSSegmentStore *store, AVD_UInt32 *currentSegmentIds)
{
    AVD_ASSERT(store != NULL);
    AVD_ASSERT(currentSegmentIds != NULL);

    AVD_UInt32 result = 0;

    for (AVD_UInt32 i = 0; i < store->sourceCount; i++) {
        AVD_HLSSegmentRing *ring    = &store->rings[i];
        AVD_UInt32 currentSegmentId = currentSegmentIds[i];
        uint8_t data                = 0x0;

        picoThreadMutexLock(ring->mutex, PICO_THREAD_INFINITE);

        for (AVD_UInt32 j = 0; j < 8; j++) {
            AVD_UInt32 checkSegmentId = currentSegmentId + j;
            AVD_UInt32 slotIndex      = checkSegmentId % AVD_HLS_SEGMENT_RING_SIZE;
            AVD_HLSSegmentSlot *slot  = &ring->slots[slotIndex];
            if (slot->segmentId == checkSegmentId && (slot->state == AVD_HLS_SEGMENT_STATE_READY || slot->state == AVD_HLS_SEGMENT_STATE_PLAYING)) {
                data |= (1 << (j + 1));
            }
        }

        picoThreadMutexUnlock(ring->mutex);

        result |= (AVD_UInt32)data << (i * 8);
    }

    return result;
}
