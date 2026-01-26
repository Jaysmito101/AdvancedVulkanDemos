#ifndef AVD_VULKAN_VIDEO_H
#define AVD_VULKAN_VIDEO_H

#include "core/avd_aligned_buffer.h"
#include "core/avd_types.h"
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
    bool isReferenceFrame;

    AVD_UInt32 pictureOrderCount;
    AVD_UInt32 topFieldOrderCount;
    AVD_UInt32 bottomFieldOrderCount;
    bool complementaryFieldPair;
} AVD_H264VideoFrameInfo;

typedef struct {
    AVD_Size bufferOffset;    // offset to start parsing from
    AVD_Size frameDataAlignment; // alignment for frame data allocations
} AVD_H264VideoLoadParams;

typedef struct {
    struct {
        AVD_UInt32 prevPicOrderCntLsb;
        AVD_Int32 prevPicOrderCntMsb;
    } type0;
    struct {
        AVD_UInt32 prevFrameNum;
        AVD_UInt32 prevFrameNumOffset;
    } type1;
    struct {
        AVD_UInt32 prevFrameNum;
        AVD_UInt32 prevFrameNumOffset;
    } type2;
} AVD_H264VideoPictureOrderCountState;

typedef struct {
    AVD_AlignedBuffer sliceDataBuffer;

    AVD_List frameInfos;
    AVD_List sliceHeaders;

    picoH264PictureParameterSet* ppsArray;
    AVD_UInt32 ppsHash;

    picoH264SequenceParameterSet* spsArray;
    AVD_UInt32 spsHash;

    AVD_H264VideoPictureOrderCountState pocState;
} AVD_H264VideoChunk;

typedef struct {
    AVD_Size byteOffset;
    AVD_Float timestampSeconds;
    AVD_Size poc;
} AVD_H264VideoSeekInfo;

typedef struct {
    picoH264SequenceParameterSet sps[PICO_H264_MAX_SPS_COUNT];
    AVD_UInt32 spsHash;

    picoH264PictureParameterSet pps[PICO_H264_MAX_PPS_COUNT];
    AVD_UInt32 ppsHash;

    AVD_List seekPoints;

    AVD_H264VideoChunk currentChunk;

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

    AVD_Float timestampSinceStartSeconds;

    picoH264Bitstream bitstream;
} AVD_H264Video;

typedef struct {
    VkVideoSessionKHR session;
    VkVideoSessionParametersKHR sessionParameters;

    VkDeviceMemory memory[128];
    AVD_UInt32 memoryAllocationCount;

    AVD_H264Video* h264Video;
} AVD_VulkanVideoDecoder;

bool avdH264VideoLoadParamsDefault(AVD_Vulkan* vulkan, AVD_H264VideoLoadParams *outParams);

bool avdH264VideoLoadFromBuffer(const uint8_t *buffer, size_t bufferSizem, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo);
bool avdH264VideoLoadFromFile(const char *filename, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo);
void avdH264VideoDestroy(AVD_H264Video *video);
void avdH264VideoDebugPrint(AVD_H264Video *video);
void avdH264VideoChunkDebugPrint(AVD_H264VideoChunk *chunk, bool logFrameInfos);
// load next chunk of NAL units, till the next IDR frame or till end of stream
// it will be loaded into the internal currentChunk member
// the current chunk is managed by the video object and will be reset on each call
bool avdH264VideoLoadChunk(AVD_H264Video *video, AVD_H264VideoChunk** outChunk);

bool avdVulkanVideoDecoderCreate(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_H264Video *h264Video);
void avdVulkanVideoDecoderDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video);


#endif // AVD_VULKAN_VIDEO_H
