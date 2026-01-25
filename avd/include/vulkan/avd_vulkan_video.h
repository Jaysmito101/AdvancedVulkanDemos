#ifndef AVD_VULKAN_VIDEO_H
#define AVD_VULKAN_VIDEO_H

#include "math/avd_math_base.h"
#include "pico/picoH264.h"
#include "vulkan/avd_vulkan_base.h"

#ifndef AVD_VULKAN_VIDEO_MAX_WIDTH
#define AVD_VULKAN_VIDEO_MAX_WIDTH 3840
#endif

#ifndef AVD_VULKAN_VIDEO_MAX_HEIGHT
#define AVD_VULKAN_VIDEO_MAX_HEIGHT 2160
#endif

#ifndef AVD_VULKAN_VIDEO_MAX_NAL_TEMP_BUFFER_SIZE
#define AVD_VULKAN_VIDEO_MAX_NAL_TEMP_BUFFER_SIZE (1024 * 1024 * 16) // 16 MB
#endif

typedef struct {
    AVD_Size offset;
    AVD_Size size;
    AVD_Float timestampSeconds;
    AVD_Float durationSeconds;
    AVD_UInt32 referencePriority;
    AVD_UInt32 pos;
    AVD_UInt32 gop;
    AVD_UInt32 displayOrder;
    bool isIntraFrame;
} AVD_H264VideoFrameInfo;

typedef struct {
    AVD_Size bufferOffset;    // offset to start parsing from
} AVD_H264VideoLoadParams;


typedef struct {
    picoH264SequenceParameterSet sps[PICO_H264_MAX_SPS_COUNT];

    picoH264PictureParameterSet pps[PICO_H264_MAX_PPS_COUNT];

    AVD_UInt32 spsHash;
    AVD_UInt32 ppsHash;

    uint8_t* nalUnitBuffer;
    uint8_t* nalUnitPayloadBuffer;


    AVD_UInt32 width;
    AVD_UInt32 height;

    AVD_UInt32 paddedWidth;
    AVD_UInt32 paddedHeight;

    AVD_UInt32 numDPBSlots;
    uint64_t framerateRaw;
    AVD_Float framerate;
    AVD_Float frameDurationSeconds;

    picoH264Bitstream bitstream;
} AVD_H264Video;

typedef struct {
    VkVideoSessionKHR session;
    VkVideoSessionParametersKHR sessionParameters;

    VkDeviceMemory memory[128];
    AVD_UInt32 memoryAllocationCount;

    AVD_H264Video* h264Video;
} AVD_VulkanVideoDecoder;

bool avdH264VideoLoadParamsDefault(AVD_H264VideoLoadParams *outParams);

bool avdH264VideoLoadFromBuffer(const uint8_t *buffer, size_t bufferSizem, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo);
bool avdH264VideoLoadFromFile(const char *filename, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo);
void avdH264VideoDestroy(AVD_H264Video *video);
void avdH264VideoDebugPrint(AVD_H264Video *video);

bool avdVulkanVideoDecoderCreate(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_H264Video *h264Video);
void avdVulkanVideoDecoderDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video);

#endif // AVD_VULKAN_VIDEO_H
