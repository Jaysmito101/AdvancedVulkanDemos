#ifndef AVD_VULKAN_VIDEO_DECODER_H
#define AVD_VULKAN_VIDEO_DECODER_H

#include "avd_vulkan_video_dpb.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/avd_vulkan_image.h"
#include "vulkan/video/avd_vulkan_video_core.h"
#include "vulkan/video/avd_vulkan_video_h264_data.h"


#ifndef AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES 
#define AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES 8
#endif


typedef struct {
    bool initialized;
    bool inUse;
    AVD_VulkanImage image;
    AVD_VulkanImageYCbCrSubresource ycbcrSubresource;

    AVD_Float timestampSeconds;
    AVD_Size chunkDisplayOrder;
    AVD_Size absoluteDisplayOrder;
} AVD_VulkanVideoDecodedFrame;

typedef struct {
    AVD_Bool ready;
    AVD_H264VideoChunk* videoChunk;
    AVD_Float timestampSeconds;
    AVD_Size chunkDisplayOrderOffset;

    AVD_Size currentSliceIndex;
} AVD_VulkanVideoDecoderChunk;

typedef struct {
    bool initialized;
    VkVideoSessionKHR session;
    VkVideoSessionParametersKHR sessionParameters;
    VkDeviceMemory memory[128];
    AVD_UInt32 memoryAllocationCount;

    AVD_VulkanBuffer bitstreamBuffer;
    AVD_VulkanVideoDPB dpb;

    AVD_VulkanVideoDecodedFrame decodedFrames[AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES];

    AVD_H264Video* h264Video;
    AVD_VulkanVideoDecoderChunk currentChunk;

    AVD_Size displayOrderOffset;
    AVD_Float timestampSecondsOffset;

    char label[64];
} AVD_VulkanVideoDecoder;

bool avdVulkanVideoDecodedFrameCreate(
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDecodedFrame *frame,
    AVD_UInt32 width,
    AVD_UInt32 height,
    VkFormat format,
    const char* label);
void avdVulkanVideoDecodedFrameDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDecodedFrame *frame);

bool avdVulkanVideoDecoderCreate(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_H264Video *h264Video, const char* label);
void avdVulkanVideoDecoderDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video);
AVD_Size avdVulkanVideoDecoderGetNumDecodedFrames(AVD_VulkanVideoDecoder *video);
bool avdVulkanVideoDecoderChunkHasFrames(AVD_VulkanVideoDecoder *video);
bool avdVulkanVideoDecoderIsChunkOutdated(AVD_VulkanVideoDecoder *video, AVD_Float videoTime);
bool avdVulkanVideoDecoderNextChunk(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_H264VideoLoadParams* chunkLoadParams, bool *eof);
bool avdVulkanVideoDecoderDecodeFrame(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, VkSemaphore signalSemaphore, VkFence signalFence);



#endif // AVD_VULKAN_VIDEO_DECODER_H