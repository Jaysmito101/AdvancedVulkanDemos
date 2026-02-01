#include "core/avd_types.h"
#include "pico/picoStream.h"
#include "scenes/hls_player/avd_scene_hls_stream.h"

#include "core/avd_base.h"

typedef struct {
    uint8_t *buffer;
    AVD_Size capacity;
    AVD_Size readHead;
    AVD_Size writeHead;
    AVD_Int64 headOffset;
} AVD_ScenesHLSStream;

static size_t __avdHLSStreamRead(void *userData, void *buffer, size_t size)
{
    AVD_ScenesHLSStream *hlsStream = (AVD_ScenesHLSStream *)userData;
    AVD_ASSERT(hlsStream != NULL);
    AVD_ASSERT(buffer != NULL || size == 0);

    AVD_Size available = hlsStream->writeHead - hlsStream->readHead;
    size               = AVD_MIN(size, available);
    if (size == 0) {
        return 0;
    }

    memcpy(buffer, hlsStream->buffer + hlsStream->readHead, size);
    hlsStream->readHead += size;

    return size;
}

static int __avdHLSStreamSeek(void *userData, int64_t offset, picoStreamSeekOrigin origin)
{
    AVD_ScenesHLSStream *hlsStream = (AVD_ScenesHLSStream *)userData;
    AVD_ASSERT(hlsStream != NULL);

    switch (origin) {
        case PICO_STREAM_SEEK_SET: {
            if (offset < hlsStream->headOffset) {
                // headOffset is the absolute position of the start of the buffer
                // cannot seek before that
                return -1;
            }

            if (offset - (AVD_Int64)hlsStream->headOffset > (AVD_Int64)hlsStream->writeHead) {
                // cannot seek beyond write head
                return -1;
            }

            hlsStream->readHead = (AVD_Size)(offset - (AVD_Int64)hlsStream->headOffset);
            break;
        }
        case PICO_STREAM_SEEK_CUR: {
            AVD_Int64 targetPos = (AVD_Int64)hlsStream->readHead + offset;
            if (targetPos < (AVD_Int64)0 || targetPos > (AVD_Int64)hlsStream->writeHead) {
                return -1;
            }

            hlsStream->readHead = (AVD_Size)targetPos;
            break;
        }
        case PICO_STREAM_SEEK_END: {
            AVD_LOG_WARN("Seeking from end not supported in HLS stream");
            return -1;
        }
        default:
            return -1;
    }

    return 0;
}

static int64_t __avdHLSStreamTell(void *userData)
{
    AVD_ScenesHLSStream *hlsStream = (AVD_ScenesHLSStream *)userData;
    AVD_ASSERT(hlsStream != NULL);
    return hlsStream->headOffset + (int64_t)hlsStream->readHead;
}

static void __avdHLSStreamDestroy(void *userData)
{
    AVD_ScenesHLSStream *hlsStream = (AVD_ScenesHLSStream *)userData;
    AVD_ASSERT(hlsStream != NULL);
    if (hlsStream->buffer) {
        AVD_FREE(hlsStream->buffer);
        hlsStream->buffer = NULL;
    }
    AVD_FREE(hlsStream);
}

picoStream avdHLSStreamCreate()
{
    picoStreamCustom_t customStream = {0};
    AVD_ScenesHLSStream *hlsStream  = (AVD_ScenesHLSStream *)AVD_MALLOC(sizeof(AVD_ScenesHLSStream));
    if (!hlsStream) {
        return NULL;
    }

    memset(hlsStream, 0, sizeof(AVD_ScenesHLSStream));
    hlsStream->capacity = AVD_HLS_STREAM_INITIAL_CAPACITY;
    hlsStream->buffer   = (uint8_t *)AVD_MALLOC(hlsStream->capacity);
    if (!hlsStream->buffer) {
        AVD_FREE(hlsStream);
        return NULL;
    }

    customStream.userData = hlsStream;
    customStream.read     = __avdHLSStreamRead;
    customStream.seek     = __avdHLSStreamSeek;
    customStream.tell     = __avdHLSStreamTell;
    customStream.destroy  = __avdHLSStreamDestroy;

    return picoStreamFromCustom(customStream, true, false);
}

bool avdHLSStreamAppendData(picoStream stream, const AVD_UInt8 *data, AVD_Size dataSize)
{
    AVD_ASSERT(stream != NULL);
    AVD_ASSERT(data != NULL || dataSize == 0);

    AVD_ScenesHLSStream *hlsStream = (AVD_ScenesHLSStream *)picoStreamGetUserData(stream);
    AVD_ASSERT(hlsStream != NULL);

    // check if there is enough room in the buffer
    AVD_Size currentSize   = hlsStream->writeHead - hlsStream->readHead + AVD_HLS_STREAM_SEEK_BUFFER_SIZE;
    AVD_Size availableSize = hlsStream->capacity - currentSize;
    AVD_Int64 startOffset  = AVD_MAX(0, (AVD_Int64)hlsStream->readHead - AVD_HLS_STREAM_SEEK_BUFFER_SIZE);
    AVD_Size moveSize      = hlsStream->writeHead - (AVD_Size)startOffset;

    if (dataSize + currentSize > AVD_HLS_STREAM_MAX_CAPACITY) {
        AVD_LOG_ERROR("Data size %zu exceeds maximum HLS stream capacity %zu", dataSize, (AVD_Size)AVD_HLS_STREAM_MAX_CAPACITY);
        return false;
    }

    if (dataSize > availableSize) {
        // need to grow the buffer
        AVD_Size newCapacity = hlsStream->capacity;
        while (dataSize > (newCapacity - currentSize)) {
            newCapacity *= AVD_HLS_STREAM_GROWTH_FACTOR;
        }

        uint8_t *newBuffer = (uint8_t *)AVD_MALLOC(newCapacity);
        if (!newBuffer) {
            AVD_LOG_ERROR("Failed to allocate memory for HLS stream growth");
            return false;
        }

        // copy existing data to new buffer
        memcpy(newBuffer, hlsStream->buffer + (AVD_Size)startOffset, moveSize);
        AVD_FREE(hlsStream->buffer);

        hlsStream->buffer   = newBuffer;
        hlsStream->capacity = newCapacity;
    } else if (startOffset > 0) {
        // move the readHead towards the beginning to make room
        memmove(hlsStream->buffer, hlsStream->buffer + (AVD_Size)startOffset, moveSize);
    }

    hlsStream->headOffset += (AVD_Int64)startOffset;
    hlsStream->readHead -= (AVD_Size)startOffset;
    hlsStream->writeHead -= (AVD_Size)startOffset;

    // append new data
    memcpy(hlsStream->buffer + hlsStream->writeHead, data, dataSize);
    hlsStream->writeHead += dataSize;

    return true;
}
