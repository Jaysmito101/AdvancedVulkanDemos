#ifndef AVD_VULKAN_VIDEO_DECODER_H
#define AVD_VULKAN_VIDEO_DECODER_H

#include "avd_vulkan_video_dpb.h"
#include "core/avd_types.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/avd_vulkan_image.h"
#include "vulkan/video/avd_vulkan_video_core.h"
#include "vulkan/video/avd_vulkan_video_h264_data.h"

#ifndef AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES
#define AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES 8
#endif

typedef enum {
    AVD_VULKAN_VIDEO_DECODED_FRAME_STATUS_FREE = 0,
    AVD_VULKAN_VIDEO_DECODED_FRAME_STATUS_PROCESSING,
    AVD_VULKAN_VIDEO_DECODED_FRAME_STATUS_READY,
    AVD_VULKAN_VIDEO_DECODED_FRAME_STATUS_ACQUIRED
} AVD_VulkanVideoDecodedFrameStatus;

typedef struct {
    AVD_Bool initialized;
    AVD_VulkanVideoDecodedFrameStatus status;

    AVD_VulkanImage image;
    AVD_VulkanImageYCbCrSubresource ycbcrSubresource;

    AVD_Size index;
    AVD_Size sliceIndex;
    AVD_Float timestampSeconds;
    AVD_Size chunkDisplayOrder;
    AVD_Size absoluteDisplayOrder;
} AVD_VulkanVideoDecodedFrame;

typedef struct {
    AVD_UInt32 bottomFieldFlag;
    AVD_UInt32 usedForLongerTermReference;
    AVD_UInt32 frameNum;
    AVD_UInt32 picOrderCount;
    AVD_UInt32 topFieldOrderCount;
    AVD_UInt32 bottomFieldOrderCount;

    AVD_Size dpbSlotIndex;
} AVD_VulkanVideoDecoderReferenceInfo;

typedef struct {
    AVD_Bool ready;
    AVD_H264VideoChunk *videoChunk;
    AVD_Float timestampSeconds;
    AVD_Size chunkDisplayOrderOffset;

    AVD_VulkanVideoDecoderReferenceInfo referenceInfo[17];
    AVD_Size currentDPBSlotIndex;

    AVD_Size references[17];
    AVD_Size referenceSlotIndex;

    AVD_Size currentSliceIndex;
} AVD_VulkanVideoDecoderChunk;

typedef struct {
    StdVideoH264PictureParameterSet* ppsArray;
    StdVideoH264ScalingLists* scalingListsArray;
    AVD_Size ppsCapacity;

    StdVideoH264SequenceParameterSet* spsArray;
    StdVideoH264SequenceParameterSetVui* vuiArray;
    StdVideoH264HrdParameters* hrdArray;
    AVD_Size spsCapacity;
} AVD_VulkanVideoDecoderVideoSessionData;

typedef struct {
    bool initialized;
    VkVideoSessionKHR session;
    VkVideoSessionParametersKHR sessionParameters;
    AVD_VulkanVideoDecoderVideoSessionData sessionData;
    VkCommandBuffer commandBuffer;
    AVD_Bool needsReset;

    VkDeviceMemory memory[128];
    AVD_UInt32 memoryAllocationCount;

    AVD_VulkanBuffer bitstreamBuffer;
    AVD_VulkanVideoDPB dpb;

    AVD_VulkanVideoDecodedFrame decodedFrames[AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES];

    AVD_H264Video *h264Video;
    AVD_VulkanVideoDecoderChunk currentChunk;

    AVD_Size displayOrderOffset;
    AVD_Float timestampSecondsOffset;

    VkFence decodeFence;

    char label[64];
} AVD_VulkanVideoDecoder;

bool avdVulkanVideoDecoderSessionDataEnsureSize(
    AVD_VulkanVideoDecoderVideoSessionData *sessionData,
    AVD_Size ppsCount,
    AVD_Size spsCount
);
void avdVulkanVideoDecoderSessionDataDestroy(AVD_VulkanVideoDecoderVideoSessionData *sessionData);

bool avdVulkanVideoDecodedFrameCreate(
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDecodedFrame *frame,
    AVD_UInt32 width,
    AVD_UInt32 height,
    VkFormat format,
    const char *label);
void avdVulkanVideoDecodedFrameDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDecodedFrame *frame);

bool avdVulkanVideoDecoderCreate(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_H264Video *h264Video, const char *label);
void avdVulkanVideoDecoderDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video);
AVD_Size avdVulkanVideoDecoderGetNumDecodedFrames(AVD_VulkanVideoDecoder *video);
bool avdVulkanVideoDecoderChunkHasFrames(AVD_VulkanVideoDecoder *video);
bool avdVulkanVideoDecoderIsChunkOutdated(AVD_VulkanVideoDecoder *video, AVD_Float videoTime);
bool avdVulkanVideoDecoderNextChunk(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_H264VideoLoadParams *chunkLoadParams, bool *eof);
bool avdVulkanVideoDecoderUpdate(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video);

bool avdVulkanVideoDecoderHasFrameForTime(
    AVD_VulkanVideoDecoder *video,
    AVD_Float currentTime);
bool avdVulkanVideoDecoderTryAcquireFrame(
    AVD_VulkanVideoDecoder *video,
    AVD_Float currentTime,
    AVD_VulkanVideoDecodedFrame **outFrame);
bool avdVulkanVideoDecoderTryDecodeFrames(
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDecoder *video);

#endif // AVD_VULKAN_VIDEO_DECODER_H