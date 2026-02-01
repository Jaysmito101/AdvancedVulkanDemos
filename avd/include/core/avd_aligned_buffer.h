#ifndef AVD_ALIGNED_BUFFER_H
#define AVD_ALIGNED_BUFFER_H

#include "avd_types.h"
#include "core/avd_base.h"

typedef struct {
    AVD_DataPtr data;
    AVD_Size size;
    AVD_Size capacity;
    AVD_Size alignment;
} AVD_AlignedBuffer;

AVD_Bool avdAlignedBufferCreate(AVD_AlignedBuffer *buffer, AVD_Size capacity, AVD_Size alignment);
void avdAlignedBufferDestroy(AVD_AlignedBuffer *buffer);
AVD_Bool avdAlignedBufferEmplace(AVD_AlignedBuffer *buffer, AVD_Size size, AVD_DataPtr *dataOut);
AVD_Bool avdAlignedBufferClear(AVD_AlignedBuffer *buffer);
AVD_Bool avdAlignedBufferTighten(AVD_AlignedBuffer *buffer);

#endif // AVD_ALIGNED_BUFFER_H