#include "core/avd_aligned_buffer.h"

AVD_Bool avdAlignedBufferCreate(AVD_AlignedBuffer *buffer, AVD_Size capacity, AVD_Size alignment)
{
    AVD_ASSERT(buffer != NULL);
    AVD_ASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0); // ensure alignment is power of two

    buffer->alignment = alignment;
    buffer->capacity  = AVD_ALIGN(capacity, alignment);
    buffer->size      = 0;
    buffer->data      = AVD_ALIGNED_ALLOC(alignment, buffer->capacity);

    AVD_CHECK_MSG(buffer->data != NULL, "Failed to allocate aligned buffer");
    return true;
}

void avdAlignedBufferDestroy(AVD_AlignedBuffer *buffer)
{
    if (buffer == NULL || buffer->data == NULL) {
        return;
    }

    AVD_ALIGNED_FREE(buffer->data);

    buffer->data      = NULL;
    buffer->size      = 0;
    buffer->capacity  = 0;
    buffer->alignment = 0;
}

AVD_Bool avdAlignedBufferEmplace(AVD_AlignedBuffer *buffer, AVD_Size size, AVD_DataPtr *dataOut)
{
    AVD_ASSERT(buffer != NULL);
    AVD_ASSERT(dataOut != NULL);

    AVD_Size alignedSize = AVD_ALIGN(size, buffer->alignment);

    if (buffer->size + alignedSize > buffer->capacity) {
        AVD_Size newCapacity = buffer->capacity * 2;
        while (newCapacity < buffer->size + alignedSize) {
            newCapacity *= 2;
        }

        AVD_DataPtr newData = AVD_ALIGNED_ALLOC(buffer->alignment, newCapacity);

        AVD_CHECK_MSG(newData != NULL, "Failed to grow aligned buffer");

        memcpy(newData, buffer->data, buffer->size);
        AVD_ALIGNED_FREE(buffer->data);

        buffer->data     = newData;
        buffer->capacity = newCapacity;
    }

    *dataOut = (AVD_UInt8 *)buffer->data + buffer->size;
    buffer->size += alignedSize;
    return true;
}

AVD_Bool avdAlignedBufferClear(AVD_AlignedBuffer *buffer)
{
    AVD_ASSERT(buffer != NULL);
    buffer->size = 0;
    return true;
}

AVD_Bool avdAlignedBufferTighten(AVD_AlignedBuffer *buffer)
{
    AVD_ASSERT(buffer != NULL);

    if (buffer->size == 0) {
        avdAlignedBufferDestroy(buffer);
        return true;
    }

    if (buffer->size == buffer->capacity) {
        return true;
    }

    AVD_Size newCapacity = AVD_ALIGN(buffer->size, buffer->alignment);
    AVD_DataPtr newData  = AVD_ALIGNED_ALLOC(buffer->alignment, newCapacity);

    AVD_CHECK_MSG(newData != NULL, "Failed to allocate tightened buffer");

    memcpy(newData, buffer->data, buffer->size);
    AVD_ALIGNED_FREE(buffer->data);

    buffer->data     = newData;
    buffer->capacity = newCapacity;
    return true;
}