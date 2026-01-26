#include "scenes/hls_player/avd_scene_hls_stream.h"

#include "core/avd_base.h"

#include <string.h>

typedef struct AVD_HLSStream {
    AVD_UInt8 *buffer;
    AVD_Size capacity;
    AVD_Size writePos;
    AVD_Size readPos;
    AVD_Size dataSize;
    picoStreamCustom_t customStream;
} AVD_HLSStream;

#define AVD_HLS_STREAM_FROM_CUSTOM(customPtr) \
    ((AVD_HLSStream *)((char *)(customPtr) - offsetof(AVD_HLSStream, customStream)))

static AVD_HLSStream *__avdHLSStreamFromPicoStream(picoStream stream)
{
    return (AVD_HLSStream *)picoStreamGetUserData(stream);
}

static bool __avdHLSStreamEnsureCapacity(AVD_HLSStream *hlsStream, AVD_Size requiredSize)
{
    if (requiredSize <= hlsStream->capacity) {
        return true;
    }

    AVD_Size newCapacity = hlsStream->capacity;
    while (newCapacity < requiredSize) {
        newCapacity *= AVD_HLS_STREAM_GROWTH_FACTOR;
    }

    AVD_UInt8 *newBuffer = (AVD_UInt8 *)AVD_MALLOC(newCapacity);
    AVD_CHECK_MSG(newBuffer != NULL, "Failed to allocate HLS stream buffer");

    if (hlsStream->dataSize > 0) {
        if (hlsStream->readPos + hlsStream->dataSize <= hlsStream->capacity) {
            memcpy(newBuffer, hlsStream->buffer + hlsStream->readPos, hlsStream->dataSize);
        } else {
            AVD_Size firstPart = hlsStream->capacity - hlsStream->readPos;
            memcpy(newBuffer, hlsStream->buffer + hlsStream->readPos, firstPart);
            memcpy(newBuffer + firstPart, hlsStream->buffer, hlsStream->dataSize - firstPart);
        }
    }

    AVD_FREE(hlsStream->buffer);
    hlsStream->buffer   = newBuffer;
    hlsStream->capacity = newCapacity;
    hlsStream->readPos  = 0;
    hlsStream->writePos = hlsStream->dataSize;

    return true;
}

static size_t __avdHLSStreamRead(void *userData, void *buffer, size_t size)
{
    AVD_HLSStream *hlsStream = (AVD_HLSStream *)userData;
    AVD_Size available       = hlsStream->dataSize;
    AVD_Size toRead          = AVD_MIN(size, available);

    if (toRead == 0) {
        return 0;
    }

    AVD_UInt8 *dst = (AVD_UInt8 *)buffer;

    if (hlsStream->readPos + toRead <= hlsStream->capacity) {
        memcpy(dst, hlsStream->buffer + hlsStream->readPos, toRead);
    } else {
        AVD_Size firstPart = hlsStream->capacity - hlsStream->readPos;
        memcpy(dst, hlsStream->buffer + hlsStream->readPos, firstPart);
        memcpy(dst + firstPart, hlsStream->buffer, toRead - firstPart);
    }

    hlsStream->readPos = (hlsStream->readPos + toRead) % hlsStream->capacity;
    hlsStream->dataSize -= toRead;

    return toRead;
}

static int __avdHLSStreamSeek(void *userData, int64_t offset, picoStreamSeekOrigin origin)
{
    AVD_HLSStream *hlsStream   = (AVD_HLSStream *)userData;
    AVD_Size basePos           = (hlsStream->writePos >= hlsStream->dataSize)
                                     ? (hlsStream->writePos - hlsStream->dataSize)
                                     : (hlsStream->capacity - (hlsStream->dataSize - hlsStream->writePos));
    AVD_Size currentReadOffset = (hlsStream->readPos >= basePos)
                                     ? (hlsStream->readPos - basePos)
                                     : (hlsStream->capacity - basePos + hlsStream->readPos);
    int64_t newPos             = 0;

    switch (origin) {
        case PICO_STREAM_SEEK_SET:
            newPos = offset;
            break;
        case PICO_STREAM_SEEK_CUR:
            newPos = (int64_t)currentReadOffset + offset;
            break;
        case PICO_STREAM_SEEK_END:
            newPos = (int64_t)hlsStream->dataSize + offset;
            break;
        default:
            return -1;
    }

    if (newPos < 0 || newPos > (int64_t)hlsStream->dataSize) {
        return -1;
    }

    hlsStream->readPos = (basePos + (AVD_Size)newPos) % hlsStream->capacity;

    return 0;
}

static int64_t __avdHLSStreamTell(void *userData)
{
    AVD_HLSStream *hlsStream   = (AVD_HLSStream *)userData;
    AVD_Size basePos           = (hlsStream->writePos >= hlsStream->dataSize)
                                     ? (hlsStream->writePos - hlsStream->dataSize)
                                     : (hlsStream->capacity - (hlsStream->dataSize - hlsStream->writePos));
    AVD_Size currentReadOffset = (hlsStream->readPos >= basePos)
                                     ? (hlsStream->readPos - basePos)
                                     : (hlsStream->capacity - basePos + hlsStream->readPos);
    return (int64_t)currentReadOffset;
}

static void __avdHLSStreamFlush(void *userData)
{
    (void)userData;
}

static void __avdHLSStreamDestroy(void *userData)
{
    AVD_HLSStream *hlsStream = (AVD_HLSStream *)userData;
    if (hlsStream->buffer) {
        AVD_FREE(hlsStream->buffer);
    }
    AVD_FREE(hlsStream);
}

picoStream avdHLSStreamCreate()
{
    AVD_Size initialCapacity = AVD_HLS_STREAM_INITIAL_CAPACITY;
    AVD_HLSStream *hlsStream = (AVD_HLSStream *)AVD_MALLOC(sizeof(AVD_HLSStream));
    AVD_ASSERT(hlsStream != NULL);

    memset(hlsStream, 0, sizeof(AVD_HLSStream));

    hlsStream->buffer = (AVD_UInt8 *)AVD_MALLOC(initialCapacity);
    AVD_ASSERT(hlsStream->buffer != NULL);

    hlsStream->capacity = initialCapacity;
    hlsStream->writePos = 0;
    hlsStream->readPos  = 0;
    hlsStream->dataSize = 0;

    hlsStream->customStream.userData = hlsStream;
    hlsStream->customStream.read     = __avdHLSStreamRead;
    hlsStream->customStream.write    = NULL;
    hlsStream->customStream.seek     = __avdHLSStreamSeek;
    hlsStream->customStream.tell     = __avdHLSStreamTell;
    hlsStream->customStream.flush    = __avdHLSStreamFlush;
    hlsStream->customStream.destroy  = __avdHLSStreamDestroy;

    return picoStreamFromCustom(&hlsStream->customStream, true, false);
}

bool avdHLSStreamAppendData(picoStream stream, const AVD_UInt8 *data, AVD_Size dataSize)
{
    AVD_ASSERT(stream != NULL);
    AVD_ASSERT(data != NULL || dataSize == 0);

    if (dataSize == 0) {
        return true;
    }

    AVD_HLSStream *hlsStream = __avdHLSStreamFromPicoStream(stream);
    AVD_ASSERT(hlsStream != NULL);

    if (!__avdHLSStreamEnsureCapacity(hlsStream, hlsStream->dataSize + dataSize)) {
        return false;
    }

    if (hlsStream->writePos + dataSize <= hlsStream->capacity) {
        memcpy(hlsStream->buffer + hlsStream->writePos, data, dataSize);
    } else {
        AVD_Size firstPart = hlsStream->capacity - hlsStream->writePos;
        memcpy(hlsStream->buffer + hlsStream->writePos, data, firstPart);
        memcpy(hlsStream->buffer, data + firstPart, dataSize - firstPart);
    }

    hlsStream->writePos = (hlsStream->writePos + dataSize) % hlsStream->capacity;
    hlsStream->dataSize += dataSize;

    return true;
}
