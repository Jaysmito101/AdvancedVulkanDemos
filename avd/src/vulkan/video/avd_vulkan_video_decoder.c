#include "vulkan/video/avd_vulkan_video_decoder.h"
#include "core/avd_base.h"
#include "math/avd_math_base.h"
#include "pico/picoPerf.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/video/avd_vulkan_video_dpb.h"
#include <stdbool.h>

static bool __avdVulkanVideoDecoderCreateSession(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video)
{
    VkVideoDecodeH264ProfileInfoKHR h264DecodeProfileInfo = {
        .sType         = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR,
        .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH,
        .pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR,
        .pNext         = NULL,
    };

    VkVideoProfileInfoKHR videoProfileInfo = {
        .sType               = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
        .videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR,
        .lumaBitDepth        = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
        .chromaBitDepth      = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
        .chromaSubsampling   = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
        .pNext               = &h264DecodeProfileInfo,
    };

    VkVideoSessionCreateInfoKHR videoSessionCreateInfo = {
        .sType                      = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR,
        .queueFamilyIndex           = vulkan->videoDecodeQueueFamilyIndex,
        .maxActiveReferencePictures = AVD_MIN(vulkan->supportedFeatures.videoCapabilitiesDecode.maxActiveReferencePictures, 16),
        .maxDpbSlots                = AVD_MIN(vulkan->supportedFeatures.videoCapabilitiesDecode.maxDpbSlots, 17),
        .maxCodedExtent             = (VkExtent2D){
                        .width  = AVD_MIN(vulkan->supportedFeatures.videoCapabilitiesDecode.maxCodedExtent.width, AVD_VULKAN_VIDEO_MAX_WIDTH),
                        .height = AVD_MIN(vulkan->supportedFeatures.videoCapabilitiesDecode.maxCodedExtent.height, AVD_VULKAN_VIDEO_MAX_HEIGHT),
        },
        .pictureFormat          = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, // NOTE: as for now there is no need to support any other formats
        .referencePictureFormat = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        .pVideoProfile          = &videoProfileInfo,
        .pStdHeaderVersion      = &vulkan->supportedFeatures.videoCapabilitiesDecode.stdHeaderVersion,
        .flags                  = 0,
    };

    AVD_CHECK_VK_RESULT(vkCreateVideoSessionKHR(vulkan->device, &videoSessionCreateInfo, NULL, &video->session), "Failed to create video session");

    AVD_UInt32 memoryRequirementsCount                          = 0;
    VkVideoSessionMemoryRequirementsKHR memoryRequirements[128] = {0};

    vkGetVideoSessionMemoryRequirementsKHR(vulkan->device, video->session, &memoryRequirementsCount, NULL);
    AVD_CHECK_MSG(memoryRequirementsCount <= AVD_ARRAY_COUNT(memoryRequirements), "Too many video session memory requirements\n");

    for (AVD_UInt32 i = 0; i < memoryRequirementsCount; ++i) {
        memoryRequirements[i] = (VkVideoSessionMemoryRequirementsKHR){
            .sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR,
            .pNext = NULL,
        };
    }
    vkGetVideoSessionMemoryRequirementsKHR(vulkan->device, video->session, &memoryRequirementsCount, memoryRequirements);

    VkBindVideoSessionMemoryInfoKHR bindMemoryInfo[128] = {0};

    for (AVD_UInt32 i = 0; i < memoryRequirementsCount; ++i) {
        VkMemoryAllocateInfo allocInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memoryRequirements[i].memoryRequirements.size,
            .memoryTypeIndex = avdVulkanFindMemoryType(vulkan, memoryRequirements[i].memoryRequirements.memoryTypeBits, 0),
        };

        AVD_CHECK_MSG(allocInfo.memoryTypeIndex != UINT32_MAX, "Failed to find suitable memory type for video session memory!");

        AVD_CHECK_VK_RESULT(vkAllocateMemory(vulkan->device, &allocInfo, NULL, &video->memory[i]), "Failed to allocate %d th video session memory", i);

        bindMemoryInfo[i] = (VkBindVideoSessionMemoryInfoKHR){
            .sType           = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR,
            .memoryBindIndex = memoryRequirements[i].memoryBindIndex,
            .memory          = video->memory[i],
            .memoryOffset    = 0,
            .memorySize      = memoryRequirements[i].memoryRequirements.size,
            .pNext           = NULL,
        };
        AVD_LOG_INFO("Allocated %llu bytes for video session memory %d", (unsigned long long)memoryRequirements[i].memoryRequirements.size, i);
    }
    video->memoryAllocationCount = memoryRequirementsCount;

    AVD_CHECK_VK_RESULT(vkBindVideoSessionMemoryKHR(vulkan->device, video->session, memoryRequirementsCount, bindMemoryInfo), "Failed to bind video session memory");

    return true;
}

static bool __avdVulkanVideoDecoderUpdateChunkBitstreamBuffer(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_VulkanVideoDecoderChunk *chunk)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(chunk != NULL);
    AVD_ASSERT(chunk->videoChunk != NULL);

    // first check if the bitstream buffer is large enough to hold the new chunk
    if (video->bitstreamBuffer.size < chunk->videoChunk->sliceDataBuffer.size) {
        if (video->bitstreamBuffer.buffer != VK_NULL_HANDLE) {
            avdVulkanBufferDestroy(vulkan, &video->bitstreamBuffer);
        }
        char bufferLabel[64];
        snprintf(bufferLabel, sizeof(bufferLabel), "%s/BitstreamBuffer", video->label);
        AVD_CHECK(
            avdVulkanBufferCreate(
                vulkan,
                &video->bitstreamBuffer,
                chunk->videoChunk->sliceDataBuffer.size,
                VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                bufferLabel));
        AVD_LOG_VERBOSE("Created bitstream buffer of size %llu bytes for video decoder", (unsigned long long)video->bitstreamBuffer.size);
    }

    AVD_CHECK(avdVulkanBufferUpload(
        vulkan,
        &video->bitstreamBuffer,
        chunk->videoChunk->sliceDataBuffer.data,
        chunk->videoChunk->sliceDataBuffer.size));

    return true;
}

static bool __avdVulkanVideoDecoderUpdateDPB(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_VulkanVideoDecoderChunk *chunk)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(chunk != NULL);
    AVD_ASSERT(chunk->videoChunk != NULL);

    if (!video->dpb.initialized || video->dpb.numDPBSlots < video->h264Video->numDPBSlots || video->dpb.numDPBSlots < video->h264Video->numDPBSlots || video->dpb.width != video->h264Video->width || video->dpb.height != video->h264Video->height) {
        if (video->dpb.initialized) {
            avdVulkanVideoDPBDestroy(vulkan, &video->dpb);
        }

        char dpbLabel[64];
        snprintf(dpbLabel, sizeof(dpbLabel), "%s/DPB", video->label);
        AVD_CHECK(
            avdVulkanVideoDecodeDPBCreate(
                vulkan,
                &video->dpb,
                video->h264Video->width,
                video->h264Video->height,
                dpbLabel));
        AVD_LOG_VERBOSE("Created DPB with %u slots for video decoder", video->dpb.numDPBSlots);
    }

    return true;
}

static bool __avdVulkanVideoDecoderPrepareForNewChunk(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_VulkanVideoDecoderChunk *chunk)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(vulkan != NULL);

    // reset current slice index
    video->currentChunk.currentSliceIndex = 0;

    // push the data
    AVD_CHECK(__avdVulkanVideoDecoderUpdateChunkBitstreamBuffer(vulkan, video, chunk));
    AVD_CHECK(__avdVulkanVideoDecoderUpdateDPB(vulkan, video, chunk));

    chunk->ready = true;
    return true;
}

bool avdVulkanVideoDecoderCreate(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_H264Video *h264Video, const char *label)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(h264Video != NULL);

    memset(video, 0, sizeof(AVD_VulkanVideoDecoder));

    if (!vulkan->supportedFeatures.videoDecode) {
        AVD_LOG_ERROR("Vulkan video decode not supported on this device");
        return false;
    }

    video->h264Video = h264Video;
    AVD_CHECK(__avdVulkanVideoDecoderCreateSession(vulkan, video));

    strncpy(video->label, label, sizeof(video->label) - 1);
    video->label[sizeof(video->label) - 1] = '\0';

    video->currentChunk.ready = false;
    video->initialized        = true;

    return true;
}

void avdVulkanVideoDecoderDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(vulkan != NULL);

    avdVulkanBufferDestroy(vulkan, &video->bitstreamBuffer);
    avdVulkanVideoDPBDestroy(vulkan, &video->dpb);
    vkDestroyVideoSessionKHR(vulkan->device, video->session, NULL);
    for (AVD_UInt32 i = 0; i < video->memoryAllocationCount; ++i) {
        vkFreeMemory(vulkan->device, video->memory[i], NULL);
    }
    avdH264VideoDestroy(video->h264Video);
    memset(video, 0, sizeof(AVD_VulkanVideoDecoder));
}

AVD_Size avdVulkanVideoDecoderGetNumDecodedFrames(AVD_VulkanVideoDecoder *video)
{
    AVD_ASSERT(video != NULL);

    AVD_Size count = 0;
    for (AVD_Size i = 0; i < AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES; i++) {
        if (video->decodedFrames[i].inUse) {
            count++;
        }
    }
    return count;
}

bool avdVulkanVideoDecoderChunkHasFrames(AVD_VulkanVideoDecoder *video)
{
    AVD_ASSERT(video != NULL);

    // check is the current slice index is less than the number of frames in the chunk
    if (video->currentChunk.videoChunk == NULL) {
        return false;
    }
    return video->currentChunk.currentSliceIndex < video->currentChunk.videoChunk->frameInfos.count;
}

bool avdVulkanVideoDecoderIsChunkOutdated(AVD_VulkanVideoDecoder *video, AVD_Float videoTime)
{
    AVD_ASSERT(video != NULL);

    if (video->currentChunk.videoChunk == NULL) {
        return true;
    }

    return video->currentChunk.timestampSeconds + video->currentChunk.videoChunk->durationSeconds < videoTime;
}

bool avdVulkanVideoDecoderNextChunk(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_H264VideoLoadParams *chunkLoadParams, bool *eof)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(eof != NULL);

    // first reset current chunk
    video->currentChunk.ready                   = false;
    video->currentChunk.videoChunk              = NULL;
    video->currentChunk.currentSliceIndex       = 0;
    video->currentChunk.timestampSeconds        = video->timestampSecondsOffset;
    video->currentChunk.chunkDisplayOrderOffset = video->displayOrderOffset;

    AVD_CHECK(
        avdH264VideoLoadChunk(
            video->h264Video,
            chunkLoadParams,
            &video->currentChunk.videoChunk,
            eof));

    AVD_CHECK(__avdVulkanVideoDecoderPrepareForNewChunk(vulkan, video, &video->currentChunk));

    video->displayOrderOffset += video->currentChunk.videoChunk->frameInfos.count;
    video->timestampSecondsOffset += video->currentChunk.videoChunk->durationSeconds;

    return true;
}

bool avdVulkanVideoDecoderDecodeFrame(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, VkSemaphore signalSemaphore, VkFence signalFence)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_CHECK_MSG(
        video->initialized && video->currentChunk.ready,
        "Video decoder not initialized");

    AVD_CHECK_MSG(
        avdVulkanVideoDecoderChunkHasFrames(video),
        "No frames available in current chunk to decode");

    // placeholder implementation, just simulating some decoding time
    static picoPerfTime time = 0;
    if (picoPerfDurationSeconds(time, picoPerfNow()) > video->h264Video->frameDurationSeconds) {
        time = picoPerfNow();
        // AVD_LOG_VERBOSE("Decoding frame %d of current chunk", (int)video->currentChunk.currentSliceIndex);
        video->currentChunk.currentSliceIndex++;
    }

    return true;
}