#include "core/avd_aligned_buffer.h"
#include "core/avd_base.h"
#include "core/avd_list.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "math/avd_math_base.h"
#include "vulkan/avd_vulkan_video.h"

#include "pico/picoH264.h"
#include <stdint.h>

static bool __avdH264VideoAddSPS(AVD_H264Video *video, picoH264SequenceParameterSet sps)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(sps != NULL);

    uint8_t spsId = sps->seqParameterSetId;

    // check if we already have this SPS
    picoH264SequenceParameterSet target = NULL;
    if (video->sps[spsId]) {
        target = video->sps[spsId];
    } else {
        // allocate new SPS
        target            = (picoH264SequenceParameterSet)malloc(sizeof(picoH264SequenceParameterSet_t));
        video->sps[spsId] = target;
        AVD_CHECK_MSG(target != NULL, "Failed to allocate memory for SPS\n");
    }

    memcpy(target, sps, sizeof(picoH264SequenceParameterSet_t));

    AVD_UInt32 spsHash = 1223456789; // seed
    for (AVD_Size i = 0; i < PICO_H264_MAX_SPS_COUNT; ++i) {
        if (video->sps[i]) {
            spsHash ^= avdHashBuffer(video->sps[i], sizeof(picoH264SequenceParameterSet_t));
        }
    }
    bool changed   = video->spsHash != spsHash;
    video->spsHash = spsHash;
    return changed;
}

static bool __avdH264VideoAddPPS(AVD_H264Video *video, picoH264PictureParameterSet pps)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(pps != NULL);

    uint8_t ppsId = pps->picParameterSetId;

    // check if we already have this PPS
    picoH264PictureParameterSet target = NULL;
    if (video->pps[ppsId]) {
        target = video->pps[ppsId];
    } else {
        // allocate new PPS
        target            = (picoH264PictureParameterSet)malloc(sizeof(picoH264PictureParameterSet_t));
        video->pps[ppsId] = target;
        AVD_CHECK_MSG(target != NULL, "Failed to allocate memory for PPS\n");
    }

    memcpy(target, pps, sizeof(picoH264PictureParameterSet_t));

    AVD_UInt32 ppsHash = 987654321; // seed
    for (AVD_Size i = 0; i < PICO_H264_MAX_PPS_COUNT; ++i) {
        if (video->pps[i]) {
            ppsHash ^= avdHashBuffer(video->pps[i], sizeof(picoH264PictureParameterSet_t));
        }
    }
    bool changed   = video->ppsHash != ppsHash;
    video->ppsHash = ppsHash;
    return changed;
}

static bool __avdH264VideoSPSUpdated(AVD_H264Video *video, uint8_t spsId)
{
    AVD_ASSERT(video != NULL);

    picoH264SequenceParameterSet sps = video->sps[spsId];
    AVD_CHECK_MSG(sps != NULL, "SPS with id %d not found", spsId);

    AVD_UInt32 newWidth        = (AVD_UInt32)(((sps->picWidthInMbsMinus1 + 1) * 16) - sps->frameCropLeftOffset * 2 - sps->frameCropRightOffset * 2);
    AVD_UInt32 newHeight       = ((2 - (AVD_UInt32)sps->frameMbsOnlyFlag) * (AVD_UInt32)(sps->picHeightInMapUnitsMinus1 + 1) * 16) - (AVD_UInt32)(sps->frameCropTopOffset * 2) - (AVD_UInt32)(sps->frameCropBottomOffset * 2);
    AVD_UInt32 newPaddedWidth  = (AVD_UInt32)(sps->picWidthInMbsMinus1 + 1) * 16;
    AVD_UInt32 newPaddedHeight = (AVD_UInt32)(sps->picHeightInMapUnitsMinus1 + 1) * 16;
    AVD_UInt32 newNumDPBSlots  = AVD_MAX(video->numDPBSlots, sps->maxNumRefFrames + 1);

    // AVD_UInt32 framerate =
    if (sps->vuiParametersPresentFlag && sps->vui.timingInfoPresentFlag && sps->vui.numUnitsInTick > 0 && sps->vui.timeScale > sps->vui.numUnitsInTick) {
        AVD_Float fps               = (AVD_Float)sps->vui.timeScale / (AVD_Float)(2 * sps->vui.numUnitsInTick);
        video->framerateRaw         = ((uint64_t)sps->vui.timeScale << 32) / (2 * (uint64_t)sps->vui.numUnitsInTick);
        video->framerate            = fps;
        video->frameDurationSeconds = 1.0f / fps;
    }

    // if any of these values have changed, then its and error as we dont support dynamic changes yet
    AVD_CHECK_MSG(video->width == 0 || video->width == newWidth, "Dynamic change of video width not supported yet");
    AVD_CHECK_MSG(video->height == 0 || video->height == newHeight, "Dynamic change of video height not supported yet");
    AVD_CHECK_MSG(video->paddedWidth == 0 || video->paddedWidth == newPaddedWidth, "Dynamic change of video padded width not supported yet");
    AVD_CHECK_MSG(video->paddedHeight == 0 || video->paddedHeight == newPaddedHeight, "Dynamic change of video padded height not supported yet");
    AVD_CHECK_MSG(video->numDPBSlots == 0 || video->numDPBSlots == newNumDPBSlots, "Dynamic change of video num DPB slots not supported yet");

    video->width        = newWidth;
    video->height       = newHeight;
    video->paddedWidth  = newPaddedWidth;
    video->paddedHeight = newPaddedHeight;
    video->numDPBSlots  = newNumDPBSlots;

    return true;
}

static bool __avdH264VideoPeekNextNalUnit(AVD_H264Video *video, picoH264NALUnitHeader outNalUnitHeader)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(outNalUnitHeader != NULL);

    AVD_Size currentCursor = video->bitstream->tell(video->bitstream->userData);

    AVD_Size nalUnitSize = 0;
    AVD_CHECK(picoH264FindNextNALUnit(video->bitstream, &nalUnitSize));

    AVD_CHECK(
        picoH264ReadNALUnit(
            video->bitstream,
            video->nalUnitBuffer,
            AVD_VULKAN_VIDEO_MAX_NAL_TEMP_BUFFER_SIZE,
            nalUnitSize));

    AVD_Size nalUnitPayloadSize = 0;
    AVD_CHECK(
        picoH264ParseNALUnit(
            video->nalUnitBuffer,
            nalUnitSize,
            outNalUnitHeader,
            video->nalUnitPayloadBuffer,
            &nalUnitPayloadSize));

    video->bitstream->seek(video->bitstream->userData, currentCursor, SEEK_SET);

    return true;
}

static bool __avdH264VideoParseNextNalUnit(AVD_H264Video *video, picoH264NALUnitHeader outNalUnitHeader, bool *spsDirty, bool *ppsDirty)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(outNalUnitHeader != NULL);

    AVD_Size nalUnitSize = 0;
    AVD_CHECK(picoH264FindNextNALUnit(video->bitstream, &nalUnitSize));

    AVD_CHECK(
        picoH264ReadNALUnit(
            video->bitstream,
            video->nalUnitBuffer,
            AVD_VULKAN_VIDEO_MAX_NAL_TEMP_BUFFER_SIZE,
            nalUnitSize));

    AVD_Size nalUnitPayloadSize = 0;
    AVD_CHECK(
        picoH264ParseNALUnit(
            video->nalUnitBuffer,
            nalUnitSize,
            outNalUnitHeader,
            video->nalUnitPayloadBuffer,
            &nalUnitPayloadSize));

    switch (outNalUnitHeader->nalUnitType) {
        case PICO_H264_NAL_UNIT_TYPE_SPS: {
            picoH264SequenceParameterSet_t sps = {};
            AVD_CHECK(picoH264ParseSequenceParameterSet(
                video->nalUnitPayloadBuffer,
                nalUnitPayloadSize,
                &sps));
            if (__avdH264VideoAddSPS(video, &sps)) {
                AVD_CHECK(__avdH264VideoSPSUpdated(video, sps.seqParameterSetId));
                if (spsDirty) {
                    *spsDirty |= true;
                }
            }
            break;
        }
        case PICO_H264_NAL_UNIT_TYPE_PPS: {
            picoH264PictureParameterSet_t pps = {};
            uint8_t spsId                     = 0;
            AVD_CHECK(
                picoH264PictureParameterSetParseSPSId(
                    video->nalUnitPayloadBuffer,
                    nalUnitPayloadSize,
                    &spsId));
            picoH264SequenceParameterSet sps = video->sps[spsId];
            AVD_CHECK_MSG(sps != NULL, "Could not find referenced SPS ID %u for PPS", spsId);
            AVD_CHECK(
                picoH264ParsePictureParameterSet(
                    video->nalUnitPayloadBuffer,
                    nalUnitPayloadSize,
                    sps,
                    &pps));
            if (ppsDirty) {
                *ppsDirty |= true;
            }
            break;
        }
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_IDR: {
            break;
        }
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_NON_IDR: {
            break;
        }
        default:
            break;
    }

    avdH264VideoDebugPrint(video);

    return true;
}

static bool __avdH264VideoLoadFromBitstream(picoH264Bitstream bitstream, AVD_H264VideoLoadParams *params, AVD_H264Video *outVideo)
{
    AVD_ASSERT(bitstream != NULL);
    AVD_ASSERT(outVideo != NULL);
    AVD_ASSERT(params != NULL);

    memset(outVideo, 0, sizeof(AVD_H264Video));

    avdAlignedBufferCreate(&outVideo->currentChunk.sliceDataBuffer, 256, params->frameDataAlignment);
    avdListCreate(&outVideo->currentChunk.frameInfos, sizeof(AVD_H264VideoFrameInfo));
    avdListCreate(&outVideo->currentChunk.sliceHeaders, sizeof(picoH264SliceHeader_t));

    bitstream->seek(bitstream->userData, params->bufferOffset, SEEK_SET);

    outVideo->nalUnitBuffer = (uint8_t *)malloc(AVD_VULKAN_VIDEO_MAX_NAL_TEMP_BUFFER_SIZE);
    AVD_CHECK_MSG(outVideo->nalUnitBuffer != NULL, "Failed to allocate memory for NAL unit buffer");

    outVideo->nalUnitPayloadBuffer = (uint8_t *)malloc(AVD_VULKAN_VIDEO_MAX_NAL_TEMP_BUFFER_SIZE);
    AVD_CHECK_MSG(outVideo->nalUnitPayloadBuffer != NULL, "Failed to allocate memory for NAL unit payload buffer");

    outVideo->bitstream = bitstream;

    // initially we parse a few sps and pps units to setup the video parameters
    // dirty flags arent propagated out here, as its assumed this is the initial load
    // and the flags are always dirty
    bool spsDirty                         = false;
    bool ppsDirty                         = false;
    bool failed                           = false;
    picoH264NALUnitHeader_t nalUnitHeader = {0};

    AVD_Size currentCursor = bitstream->tell(bitstream->userData);
    while (!failed && (spsDirty == false || ppsDirty == false)) {
        failed = !__avdH264VideoParseNextNalUnit(outVideo, &nalUnitHeader, &spsDirty, &ppsDirty);
    }
    AVD_CHECK_MSG(!failed, "Failed to parse initial SPS/PPS NAL units");

    bitstream->seek(bitstream->userData, currentCursor, SEEK_SET);

    return true;
}

static bool __avdH264VideoResetChunk(AVD_H264Video *video)
{
    AVD_ASSERT(video != NULL);

    avdAlignedBufferDestroy(&video->currentChunk.sliceDataBuffer);
    avdListClear(&video->currentChunk.frameInfos);
    avdListClear(&video->currentChunk.sliceHeaders);

    return true;
}

bool avdH264VideoLoadFromBuffer(const uint8_t *buffer, size_t bufferSize, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo)
{
    AVD_ASSERT(buffer != NULL);
    AVD_ASSERT(bufferSize > 0);
    AVD_ASSERT(outVideo != NULL);
    AVD_H264Video *video = (AVD_H264Video *)malloc(sizeof(AVD_H264Video));
    AVD_CHECK_MSG(video != NULL, "Failed to allocate memory for H.264 video");
    *outVideo                   = video;
    picoH264Bitstream bitstream = picoH264BitstreamFromBuffer(buffer, bufferSize);
    AVD_CHECK_MSG(bitstream != NULL, "Failed to create bitstream from buffer");
    return __avdH264VideoLoadFromBitstream(bitstream, params, video);
}

bool avdH264VideoLoadFromFile(const char *filename, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo)
{
    AVD_ASSERT(filename != NULL);
    AVD_ASSERT(outVideo != NULL);
    AVD_H264Video *video = (AVD_H264Video *)malloc(sizeof(AVD_H264Video));
    AVD_CHECK_MSG(video != NULL, "Failed to allocate memory for H.264 video");
    *outVideo                   = video;
    picoH264Bitstream bitstream = picoH264BitstreamFromFile(filename);
    AVD_CHECK_MSG(bitstream != NULL, "Failed to create bitstream from file: %s", filename);
    return __avdH264VideoLoadFromBitstream(bitstream, params, video);
}

void avdH264VideoDestroy(AVD_H264Video *video)
{
    free(video->nalUnitBuffer);
    free(video->nalUnitPayloadBuffer);

    avdAlignedBufferDestroy(&video->currentChunk.sliceDataBuffer);
    avdListDestroy(&video->currentChunk.frameInfos);
    avdListDestroy(&video->currentChunk.sliceHeaders);

    if (video->bitstream) {
        picoH264BitstreamDestroy(video->bitstream);
    }

    for (AVD_Size i = 0; i < PICO_H264_MAX_SPS_COUNT; ++i) {
        if (video->sps[i]) {
            free(video->sps[i]);
            video->sps[i] = NULL;
        }
    }

    for (AVD_Size i = 0; i < PICO_H264_MAX_PPS_COUNT; ++i) {
        if (video->pps[i]) {
            free(video->pps[i]);
            video->pps[i] = NULL;
        }
    }

    free(video);
}

void avdH264VideoDebugPrint(AVD_H264Video *video)
{
    AVD_ASSERT(video != NULL);

    AVD_LOG_INFO("H.264 Video Debug Info:");
    AVD_LOG_INFO("  Width: %u", video->width);
    AVD_LOG_INFO("  Height: %u", video->height);
    AVD_LOG_INFO("  Padded Width: %u", video->paddedWidth);
    AVD_LOG_INFO("  Padded Height: %u", video->paddedHeight);
    AVD_LOG_INFO("  Number of DPB Slots: %u", video->numDPBSlots);
    AVD_LOG_INFO("  Framerate: %.3f fps", video->framerate);
    AVD_LOG_INFO("  Frame Duration: %.6f seconds", video->frameDurationSeconds);
}

bool avdH264VideoLoadParamsDefault(AVD_Vulkan *vulkan, AVD_H264VideoLoadParams *outParams)
{
    AVD_ASSERT(outParams != NULL);
    memset(outParams, 0, sizeof(AVD_H264VideoLoadParams));
    outParams->bufferOffset       = 0;
    outParams->frameDataAlignment = AVD_ALIGN(
        vulkan->supportedFeatures.videoCapabilitiesDecode.minBitstreamBufferOffsetAlignment,
        vulkan->supportedFeatures.videoCapabilitiesDecode.minBitstreamBufferSizeAlignment);
    return true;
}

bool avdH264VideoLoadChunk(AVD_H264Video *video, AVD_H264VideoChunk **outChunk)
{
    AVD_ASSERT(video != NULL);

    AVD_CHECK(__avdH264VideoResetChunk(video));

    picoH264NALUnitHeader_t nalUnitHeader = {0};
    bool idrEncountered                   = false;
    bool spsDirty                         = false;
    bool ppsDirty                         = false;

    do {
        AVD_Size currentCursor = video->bitstream->tell(video->bitstream->userData);
        AVD_CHECK(__avdH264VideoPeekNextNalUnit(video, &nalUnitHeader));
        if (nalUnitHeader.nalUnitType == PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_IDR) {
            if (idrEncountered) {
                break;
            }
            idrEncountered = true;
        }
        AVD_CHECK(__avdH264VideoParseNextNalUnit(video, &nalUnitHeader, &spsDirty, &ppsDirty));
    } while (true);

    *outChunk = &video->currentChunk;

    return true;
}