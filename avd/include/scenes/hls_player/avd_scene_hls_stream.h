#ifndef AVD_SCENE_HLS_STREAM_H
#define AVD_SCENE_HLS_STREAM_H

#include "core/avd_types.h"

#include "pico/picoStream.h"


#define AVD_HLS_STREAM_INITIAL_CAPACITY (1024 * 64)
#define AVD_HLS_STREAM_GROWTH_FACTOR    2

picoStream avdHLSStreamCreate();
bool avdHLSStreamAppendData(picoStream stream, const AVD_UInt8 *data, AVD_Size dataSize);

#endif // AVD_SCENE_HLS_STREAM_H