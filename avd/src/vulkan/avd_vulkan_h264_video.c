#include "core/avd_aligned_buffer.h"
#include "core/avd_base.h"
#include "core/avd_list.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "math/avd_math_base.h"
#include "vulkan/avd_vulkan_video.h"

#include "pico/picoH264.h"
#include "pico/picoStream.h"
#include <stdint.h>

static size_t __avdH264BitstreamReadCallback(void *userData, uint8_t *buffer, size_t size)
{
    picoStream stream = (picoStream)userData;
    return picoStreamRead(stream, buffer, size);
}

static bool __avdH264BitstreamSeekCallback(void *userData, int64_t offset, int origin)
{
    picoStream stream = (picoStream)userData;
    picoStreamSeekOrigin streamOrigin;
    switch (origin) {
        case SEEK_SET:
            streamOrigin = PICO_STREAM_SEEK_SET;
            break;
        case SEEK_CUR:
            streamOrigin = PICO_STREAM_SEEK_CUR;
            break;
        case SEEK_END:
            streamOrigin = PICO_STREAM_SEEK_END;
            break;
        default:
            return false;
    }
    return picoStreamSeek(stream, offset, streamOrigin) == 0;
}

static size_t __avdH264BitstreamTellCallback(void *userData)
{
    picoStream stream = (picoStream)userData;
    return (size_t)picoStreamTell(stream);
}

static void __avdH264BitstreamDestroyCallback(void *userData)
{
    picoStream stream = (picoStream)userData;
    if (stream) {
        picoStreamDestroy(stream);
    }
}

static picoH264Bitstream __avdH264BitstreamFromPicoStream(picoStream stream)
{
    picoH264Bitstream bitstream = picoH264BitstreamCreate();
    if (!bitstream) {
        return NULL;
    }
    bitstream->userData = stream;
    bitstream->read     = __avdH264BitstreamReadCallback;
    bitstream->seek     = __avdH264BitstreamSeekCallback;
    bitstream->tell     = __avdH264BitstreamTellCallback;
    bitstream->destroy  = __avdH264BitstreamDestroyCallback;
    return bitstream;
}

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
    } else {
        AVD_LOG_WARN("SPS does not contain valid VUI timing info, cannot determine framerate, gussing it to be 30 FPS");
        video->framerateRaw         = (30ULL << 32) / 1ULL;
        video->framerate            = 30.0f;
        video->frameDurationSeconds = 1.0f / 30.0f;
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
static bool __avdH264VideoPeekNextNalUnit(AVD_H264Video *video, picoH264NALUnitHeader outNalUnitHeader, bool restorePosition, bool *eof)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(outNalUnitHeader != NULL);

    AVD_Size currentCursor = video->bitstream->tell(video->bitstream->userData);

    AVD_Size nalUnitSize = 0;
    if (!picoH264FindNextNALUnit(video->bitstream, &nalUnitSize)) {
        if (eof) {
            *eof = true;
        }
        return false;
    }

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

    if (restorePosition) {
        video->bitstream->seek(video->bitstream->userData, currentCursor, SEEK_SET);
    }

    return true;
}

static bool __avdH264VideoIsMMCO5Present(picoH264SliceHeader sliceHeader, picoH264NALRefIDC nalRefIdc)
{
    AVD_ASSERT(sliceHeader != NULL);

    if (nalRefIdc == PICO_H264_NAL_REF_IDC_DISPOSABLE) {
        return false;
    }

    picoH264DecRefPicMarking marking = &sliceHeader->decRefPicMarking;
    if (marking->adaptiveRefPicMarkingModeFlag) {
        for (AVD_Size i = 0; i < marking->numMMCOOperations; ++i) {
            picoH264MMCOOperation op = &marking->mmcoOperations[i];
            if (op->memoryManagementControlOperation == 5) {
                return true;
            }
        }
    }

    return false;
}

static bool __avdH264VideoCalculatePictureOrderCountType0(AVD_H264Video *video, picoH264SequenceParameterSet sps, picoH264SliceHeader sliceHeader, AVD_H264VideoFrameInfo *outFrameInfo)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(sps != NULL);
    AVD_ASSERT(sliceHeader != NULL);
    AVD_ASSERT(outFrameInfo != NULL);

    if (!outFrameInfo->isReferenceFrame) {
        // reset poc state on IDR frames
        video->currentChunk.pocState.type0.prevPicOrderCntMsb = 0;
        video->currentChunk.pocState.type0.prevPicOrderCntLsb = 0;
    }

    // this determines the poc wrap around point (7 - 2)
    AVD_UInt32 maxPicOrderCntLsb = 1 << (sps->log2MaxPicOrderCntLsbMinus4 + 4);

    AVD_H264VideoChunk *chunk = &video->currentChunk;

    // calculate msb with wrap around detection (8-3)
    AVD_UInt32 picOrderCntMsb = 0;
    if ((sliceHeader->picOrderCntLsb < chunk->pocState.type0.prevPicOrderCntLsb) && ((chunk->pocState.type0.prevPicOrderCntLsb - sliceHeader->picOrderCntLsb) >= (maxPicOrderCntLsb / 2))) {
        picOrderCntMsb = chunk->pocState.type0.prevPicOrderCntMsb + maxPicOrderCntLsb; // wrapped forward
    } else if ((sliceHeader->picOrderCntLsb > chunk->pocState.type0.prevPicOrderCntLsb) && ((sliceHeader->picOrderCntLsb - chunk->pocState.type0.prevPicOrderCntLsb) > (maxPicOrderCntLsb / 2))) {
        picOrderCntMsb = chunk->pocState.type0.prevPicOrderCntMsb - maxPicOrderCntLsb; // wrapped backward
    } else {
        picOrderCntMsb = chunk->pocState.type0.prevPicOrderCntMsb; // no wrap
    }

    // top field order count calculation (8-4)
    if (!sliceHeader->fieldPicFlag || !sliceHeader->bottomFieldFlag) {
        outFrameInfo->topFieldOrderCount = picOrderCntMsb + sliceHeader->picOrderCntLsb;
    }

    // bottom field order count calculation (8-5)
    if (!sliceHeader->fieldPicFlag) {
        outFrameInfo->bottomFieldOrderCount = outFrameInfo->topFieldOrderCount + sliceHeader->deltaPicOrderCntBottom;
    } else if (sliceHeader->bottomFieldFlag) {
        outFrameInfo->bottomFieldOrderCount = picOrderCntMsb + sliceHeader->picOrderCntLsb;
    }

    if (__avdH264VideoIsMMCO5Present(sliceHeader, (picoH264NALRefIDC)outFrameInfo->referencePriority)) {
        chunk->pocState.type0.prevPicOrderCntMsb = 0;
        if (!sliceHeader->fieldPicFlag) {
            // set to TopFieldOrderCount after having been reset by mmco 5
            int tempPicOrderCnt                      = AVD_MIN(outFrameInfo->topFieldOrderCount, outFrameInfo->bottomFieldOrderCount);
            chunk->pocState.type0.prevPicOrderCntLsb = outFrameInfo->topFieldOrderCount - tempPicOrderCnt;
        } else {
            // note that for a top field TopFieldOrderCnt is 0 after mmco 5
            // therefore we don't have to distinguish between top and bottom fields here
            chunk->pocState.type0.prevPicOrderCntLsb = 0;
        }
    } else if (outFrameInfo->referencePriority != 0) // reference picture
    {
        chunk->pocState.type0.prevPicOrderCntMsb = picOrderCntMsb;
        chunk->pocState.type0.prevPicOrderCntLsb = sliceHeader->picOrderCntLsb;
    }

    return true;
}

static bool __avdH264VideoCalculatePictureOrderCountType1(AVD_H264Video *video, picoH264SequenceParameterSet sps, picoH264SliceHeader sliceHeader, AVD_H264VideoFrameInfo *outFrameInfo)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(sps != NULL);
    AVD_ASSERT(sliceHeader != NULL);
    AVD_ASSERT(outFrameInfo != NULL);

    AVD_UInt32 maxFrameNum = 1 << (sps->log2MaxFrameNumMinus4 + 4); // (7-1)

    AVD_H264VideoChunk *chunk = &video->currentChunk;

    // FrameNumOffset (8-6)
    AVD_UInt32 frameNumOffset = 0;
    if (!outFrameInfo->isReferenceFrame) { // (nal_unit_type == 5)
        frameNumOffset = 0;
    } else if (chunk->pocState.type1.prevFrameNum > sliceHeader->frameNum) {
        frameNumOffset = chunk->pocState.type1.prevFrameNumOffset + maxFrameNum;
    } else {
        frameNumOffset = chunk->pocState.type1.prevFrameNumOffset;
    }

    // absFrameNum (8-7)
    AVD_UInt32 absFrameNum = 0;
    if (sps->numRefFramesInPicOrderCntCycle > 0) {
        absFrameNum = frameNumOffset + sliceHeader->frameNum;
    } else {
        absFrameNum = 0;
    }

    if (outFrameInfo->referencePriority == 0 && absFrameNum > 0) {
        absFrameNum = absFrameNum - 1;
    }

    // picOrderCntCycleCnt, frameNumInPicOrderCntCycle (8-8)
    AVD_UInt32 expectedPicOrderCnt = 0;
    if (absFrameNum > 0) {
        AVD_UInt32 picOrderCntCycleCnt        = (absFrameNum - 1) / sps->numRefFramesInPicOrderCntCycle;
        AVD_UInt32 frameNumInPicOrderCntCycle = (absFrameNum - 1) % sps->numRefFramesInPicOrderCntCycle;
        // expectedDeltaPerPicOrderCntCycle (8-9)
        AVD_UInt32 expectedDeltaPerPicOrderCntCycle = 0;
        for (AVD_UInt32 i = 0; i < sps->numRefFramesInPicOrderCntCycle; i++) {
            expectedDeltaPerPicOrderCntCycle += sps->offsetForRefFrame[i];
        }
        // expectedPicOrderCnt (8-10)
        expectedPicOrderCnt = picOrderCntCycleCnt * expectedDeltaPerPicOrderCntCycle;
        for (AVD_UInt32 i = 0; i <= frameNumInPicOrderCntCycle; i++) {
            expectedPicOrderCnt = expectedPicOrderCnt + sps->offsetForRefFrame[i];
        }
    } else {
        expectedPicOrderCnt = 0;
    }

    if (outFrameInfo->referencePriority == 0) {
        expectedPicOrderCnt += sps->offsetForNonRefPic;
    }

    // TopFieldOrderCnt, BottomFieldOrderCnt (8-11)
    if (!sliceHeader->fieldPicFlag) {
        outFrameInfo->topFieldOrderCount    = expectedPicOrderCnt + sliceHeader->deltaPicOrderCnt[0];
        outFrameInfo->bottomFieldOrderCount = outFrameInfo->topFieldOrderCount + sps->offsetForTopToBottomField + sliceHeader->deltaPicOrderCnt[1];
    } else if (!sliceHeader->bottomFieldFlag) {
        outFrameInfo->topFieldOrderCount = expectedPicOrderCnt + sliceHeader->deltaPicOrderCnt[0];
    } else {
        outFrameInfo->bottomFieldOrderCount = expectedPicOrderCnt + sps->offsetForTopToBottomField + sliceHeader->deltaPicOrderCnt[0];
    }

    if (__avdH264VideoIsMMCO5Present(sliceHeader, (picoH264NALRefIDC)outFrameInfo->referencePriority)) {
        chunk->pocState.type1.prevFrameNumOffset = 0;
        chunk->pocState.type1.prevFrameNum       = 0;
    } else {
        chunk->pocState.type1.prevFrameNumOffset = frameNumOffset;
        chunk->pocState.type1.prevFrameNum       = sliceHeader->frameNum;
    }

    return true;
}

static bool __avdH264VideoCalculatePictureOrderCountType2(AVD_H264Video *video, picoH264SequenceParameterSet sps, picoH264SliceHeader sliceHeader, AVD_H264VideoFrameInfo *outFrameInfo)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(sps != NULL);
    AVD_ASSERT(sliceHeader != NULL);
    AVD_ASSERT(outFrameInfo != NULL);

    AVD_H264VideoChunk *chunk = &video->currentChunk;

    AVD_UInt32 maxFrameNum = 1 << (sps->log2MaxFrameNumMinus4 + 4); // (7-1)

    // FrameNumOffset (8-12)
    AVD_UInt32 frameNumOffset = 0;
    if (!outFrameInfo->isReferenceFrame) { // (slh->nal_unit_type == 5)
        frameNumOffset = 0;
    } else if (chunk->pocState.type2.prevFrameNum > sliceHeader->frameNum) {
        frameNumOffset = chunk->pocState.type2.prevFrameNumOffset + maxFrameNum;
    } else {
        frameNumOffset = chunk->pocState.type2.prevFrameNumOffset;
    }

    // tempPicOrderCnt (8-13)
    AVD_Int32 tempPicOrderCnt = 0;
    if (!outFrameInfo->isReferenceFrame) { // (slh->nal_unit_type == 5)
        tempPicOrderCnt = 0;
    } else if (outFrameInfo->referencePriority == 0) {
        tempPicOrderCnt = 2 * (frameNumOffset + sliceHeader->frameNum) - 1;
    } else {
        tempPicOrderCnt = 2 * (frameNumOffset + sliceHeader->frameNum);
    }

    // TopFieldOrderCnt, BottomFieldOrderCnt (8-14)
    if (!sliceHeader->fieldPicFlag) {
        outFrameInfo->topFieldOrderCount    = tempPicOrderCnt;
        outFrameInfo->bottomFieldOrderCount = tempPicOrderCnt;
    } else if (sliceHeader->bottomFieldFlag) {
        outFrameInfo->bottomFieldOrderCount = tempPicOrderCnt;
    } else {
        outFrameInfo->topFieldOrderCount = tempPicOrderCnt;
    }

    if (__avdH264VideoIsMMCO5Present(sliceHeader, (picoH264NALRefIDC)outFrameInfo->referencePriority)) {
        chunk->pocState.type2.prevFrameNumOffset = 0;
        chunk->pocState.type2.prevFrameNum       = 0;
    } else {
        chunk->pocState.type2.prevFrameNumOffset = frameNumOffset;
        chunk->pocState.type2.prevFrameNum       = sliceHeader->frameNum;
    }

    return true;
}

static bool __avdH264VideoCalculatePictureOrderCount(AVD_H264Video *video, picoH264SequenceParameterSet sps, picoH264SliceHeader sliceHeader, AVD_H264VideoFrameInfo *outFrameInfo)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(sps != NULL);
    AVD_ASSERT(sliceHeader != NULL);
    AVD_ASSERT(outFrameInfo != NULL);

    switch (sps->picOrderCntType) {
        case 0: {
            AVD_CHECK(__avdH264VideoCalculatePictureOrderCountType0(video, sps, sliceHeader, outFrameInfo));
            break;
        }
        case 1: {
            AVD_CHECK(__avdH264VideoCalculatePictureOrderCountType1(video, sps, sliceHeader, outFrameInfo));
            break;
        }
        case 2: {
            AVD_CHECK(__avdH264VideoCalculatePictureOrderCountType2(video, sps, sliceHeader, outFrameInfo));
            break;
        }
        default:
            AVD_CHECK_MSG(false, "Unsupported pic order count type: %d", sps->picOrderCntType);
            break;
    }

    if (sliceHeader->fieldPicFlag || outFrameInfo->complementaryFieldPair) {
        outFrameInfo->pictureOrderCount = AVD_MIN(
            outFrameInfo->topFieldOrderCount,
            outFrameInfo->bottomFieldOrderCount);
    } else if (!sliceHeader->bottomFieldFlag) {
        outFrameInfo->pictureOrderCount = outFrameInfo->topFieldOrderCount;
    } else {
        outFrameInfo->pictureOrderCount = outFrameInfo->bottomFieldOrderCount;
    }

    return true;
}

static bool __avdH264VideoParseNextNalUnit(AVD_H264Video *video, picoH264NALUnitHeader outNalUnitHeader, bool *spsDirty, bool *ppsDirty, bool *eof)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(outNalUnitHeader != NULL);

    AVD_Size nalUnitSize = 0;
    if (!picoH264FindNextNALUnit(video->bitstream, &nalUnitSize)) {
        if (eof) {
            *eof = true;
        }
        return false;
    }

    AVD_Size currentCursor = video->bitstream->tell(video->bitstream->userData);

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
            if (__avdH264VideoAddPPS(video, &pps)) {
                if (ppsDirty) {
                    *ppsDirty |= true;
                }
            }
            break;
        }
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_IDR:
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_NON_IDR: {
            bool isReferenceFrame = (outNalUnitHeader->nalUnitType != PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_IDR);
            uint8_t ppsId         = 0;
            uint8_t spsId         = 0;
            AVD_CHECK(picoH264SliceHeaderParsePPSId(
                video->nalUnitPayloadBuffer,
                nalUnitPayloadSize,
                &ppsId));
            picoH264PictureParameterSet pps = video->pps[ppsId];
            AVD_CHECK_MSG(pps != NULL, "Could not find referenced PPS ID %u for Slice", ppsId);
            spsId                            = pps->seqParameterSetId;
            picoH264SequenceParameterSet sps = video->sps[spsId];
            AVD_CHECK_MSG(sps != NULL, "Could not find referenced SPS ID %u for Slice", spsId);

            picoH264SliceLayerWithoutPartitioning_t slice = {};
            AVD_CHECK(
                picoH264ParseSliceLayerWithoutPartitioning(
                    video->nalUnitPayloadBuffer,
                    nalUnitPayloadSize,
                    outNalUnitHeader,
                    sps,
                    pps,
                    &slice));

            AVD_H264VideoChunk *chunk = &video->currentChunk;
            avdListPushBack(&video->currentChunk.sliceHeaders, &slice.header);
            AVD_H264VideoFrameInfo frameInfoRaw = {
                .offset            = chunk->sliceDataBuffer.size,
                .size              = AVD_ALIGN(nalUnitSize, chunk->sliceDataBuffer.alignment),
                .timestampSeconds  = video->timestampSinceStartSeconds,
                .durationSeconds   = video->frameDurationSeconds,
                .referencePriority = outNalUnitHeader->nalRefIDC,
                .isReferenceFrame  = isReferenceFrame,
            };
            AVD_H264VideoFrameInfo *frameInfo = (AVD_H264VideoFrameInfo *)avdListPushBack(
                &video->currentChunk.frameInfos,
                &frameInfoRaw);

            AVD_DataPtr dataPtr = {0};
            AVD_CHECK(avdAlignedBufferEmplace(
                &chunk->sliceDataBuffer,
                nalUnitSize,
                &dataPtr));
            memcpy(dataPtr, video->nalUnitBuffer, nalUnitSize);

            AVD_CHECK(
                __avdH264VideoCalculatePictureOrderCount(
                    video,
                    sps,
                    &slice.header,
                    frameInfo));

            if (!isReferenceFrame) {
                AVD_H264VideoSeekInfo seekInfo = {
                    .byteOffset       = currentCursor,
                    .timestampSeconds = frameInfo->timestampSeconds,
                    .poc              = frameInfo->pictureOrderCount,
                };
                avdListPushBack(&video->seekPoints, &seekInfo);
            }

            video->timestampSinceStartSeconds += video->frameDurationSeconds;

            break;
        }
        default:
            break;
    }

    return true;
}

static bool __avdH264VideoResetChunk(AVD_H264Video *video)
{
    AVD_ASSERT(video != NULL);

    avdAlignedBufferClear(&video->currentChunk.sliceDataBuffer);
    avdListClear(&video->currentChunk.frameInfos);
    avdListClear(&video->currentChunk.sliceHeaders);

    video->currentChunk.spsArray = NULL;
    video->currentChunk.spsHash  = 0;

    video->currentChunk.ppsArray = NULL;
    video->currentChunk.ppsHash  = 0;

    memset(&video->currentChunk.pocState, 0, sizeof(AVD_H264VideoPictureOrderCountState));

    return true;
}

bool avdH264VideoLoadFromStream(picoStream stream, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo)
{
    AVD_ASSERT(stream != NULL);
    AVD_ASSERT(outVideo != NULL);
    AVD_ASSERT(params != NULL);

    AVD_H264Video *video = (AVD_H264Video *)malloc(sizeof(AVD_H264Video));
    AVD_CHECK_MSG(video != NULL, "Failed to allocate memory for H.264 video");
    *outVideo = video;

    memset(video, 0, sizeof(AVD_H264Video));

    avdAlignedBufferCreate(&video->currentChunk.sliceDataBuffer, 256, params->frameDataAlignment);
    avdListCreate(&video->currentChunk.frameInfos, sizeof(AVD_H264VideoFrameInfo));
    avdListCreate(&video->currentChunk.sliceHeaders, sizeof(picoH264SliceHeader_t));

    avdListCreate(&video->seekPoints, sizeof(AVD_H264VideoSeekInfo));

    picoStreamSeek(stream, (int64_t)params->bufferOffset, PICO_STREAM_SEEK_SET);

    video->nalUnitBuffer = (uint8_t *)malloc(AVD_VULKAN_VIDEO_MAX_NAL_TEMP_BUFFER_SIZE);
    AVD_CHECK_MSG(video->nalUnitBuffer != NULL, "Failed to allocate memory for NAL unit buffer");

    video->nalUnitPayloadBuffer = (uint8_t *)malloc(AVD_VULKAN_VIDEO_MAX_NAL_TEMP_BUFFER_SIZE);
    AVD_CHECK_MSG(video->nalUnitPayloadBuffer != NULL, "Failed to allocate memory for NAL unit payload buffer");

    video->bitstream = __avdH264BitstreamFromPicoStream(stream);
    AVD_CHECK_MSG(video->bitstream != NULL, "Failed to create bitstream from picoStream");

    bool spsDirty                         = false;
    bool ppsDirty                         = false;
    bool failed                           = false;
    bool eof                              = false;
    picoH264NALUnitHeader_t nalUnitHeader = {0};

    AVD_Size currentCursor = video->bitstream->tell(video->bitstream->userData);
    while (!failed && !eof && (spsDirty == false || ppsDirty == false)) {
        if (!__avdH264VideoPeekNextNalUnit(video, &nalUnitHeader, true, &eof)) {
            failed = true;
            break;
        }

        if (nalUnitHeader.nalUnitType == PICO_H264_NAL_UNIT_TYPE_SPS ||
            nalUnitHeader.nalUnitType == PICO_H264_NAL_UNIT_TYPE_PPS) {
            if (!__avdH264VideoParseNextNalUnit(video, &nalUnitHeader, &spsDirty, &ppsDirty, &eof)) {
                failed = !eof;
                break;
            }
        } else {
            AVD_Size nalUnitSize = 0;
            AVD_CHECK(picoH264FindNextNALUnit(video->bitstream, &nalUnitSize));
            video->bitstream->seek(video->bitstream->userData, nalUnitSize, SEEK_CUR);
        }
    }
    AVD_CHECK_MSG(!failed, "Failed to parse initial SPS/PPS NAL units");

    video->bitstream->seek(video->bitstream->userData, currentCursor, SEEK_SET);

    return true;
}

bool avdH264VideoLoadFromBuffer(const uint8_t *buffer, size_t bufferSize, bool bufferOwned, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo)
{
    AVD_ASSERT(buffer != NULL);
    AVD_ASSERT(bufferSize > 0);
    AVD_ASSERT(outVideo != NULL);
    picoStream stream = picoStreamFromMemory((void *)buffer, bufferSize, true, false, bufferOwned);
    AVD_CHECK_MSG(stream != NULL, "Failed to create picoStream from buffer");
    return avdH264VideoLoadFromStream(stream, params, outVideo);
}

bool avdH264VideoLoadFromFile(const char *filename, AVD_H264VideoLoadParams *params, AVD_H264Video **outVideo)
{
    AVD_ASSERT(filename != NULL);
    AVD_ASSERT(outVideo != NULL);
    picoStream stream = picoStreamFromFileMapped(filename);
    AVD_CHECK_MSG(stream != NULL, "Failed to create picoStream from file: %s", filename);
    return avdH264VideoLoadFromStream(stream, params, outVideo);
}

void avdH264VideoDestroy(AVD_H264Video *video)
{
    free(video->nalUnitBuffer);
    free(video->nalUnitPayloadBuffer);

    avdAlignedBufferDestroy(&video->currentChunk.sliceDataBuffer);
    avdListDestroy(&video->currentChunk.frameInfos);
    avdListDestroy(&video->currentChunk.sliceHeaders);

    avdListDestroy(&video->seekPoints);

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

void avdH264VideoChunkDebugPrint(AVD_H264VideoChunk *chunk, bool logFrameInfos)
{
    AVD_ASSERT(chunk != NULL);

    AVD_LOG_INFO("H.264 Video Chunk Debug Info:");
    AVD_LOG_INFO("  Slice Data Buffer:");
    AVD_LOG_INFO("    Size: %zu bytes", chunk->sliceDataBuffer.size);
    AVD_LOG_INFO("    Alignment: %zu bytes", chunk->sliceDataBuffer.alignment);
    AVD_LOG_INFO("    Capacity: %zu bytes", chunk->sliceDataBuffer.capacity);
    AVD_LOG_INFO("  Number of Frame Infos: %zu", chunk->frameInfos.count);
    AVD_LOG_INFO("  Number of Slice Headers: %zu", chunk->sliceHeaders.count);
    AVD_LOG_INFO("  SPS Hash: 0x%08X", chunk->spsHash);
    AVD_LOG_INFO("  PPS Hash: 0x%08X", chunk->ppsHash);
    AVD_LOG_INFO("  Duration of Chunk: %.6f seconds", chunk->durationSeconds);
    AVD_LOG_INFO("  Timestamp of Chunk: %.6f seconds", chunk->timestampSeconds);
    if (logFrameInfos && chunk->frameInfos.count > 0) {
        AVD_LOG_INFO("  Frame Infos:");
        for (AVD_Size i = 0; i < chunk->frameInfos.count; ++i) {
            AVD_H264VideoFrameInfo *frameInfo = (AVD_H264VideoFrameInfo *)avdListGet(&chunk->frameInfos, i);
            AVD_LOG_INFO("    Frame %zu:", i);
            AVD_LOG_INFO("      Offset: %zu", frameInfo->offset);
            AVD_LOG_INFO("      Size: %zu", frameInfo->size);
            AVD_LOG_INFO("      Timestamp: %.6f seconds", frameInfo->timestampSeconds);
            AVD_LOG_INFO("      Duration: %.6f seconds", frameInfo->durationSeconds);
            AVD_LOG_INFO("      Is Reference Frame: %s", frameInfo->isReferenceFrame ? "Yes" : "No");
            AVD_LOG_INFO("      Reference Priority: %u", frameInfo->referencePriority);
            AVD_LOG_INFO("      Picture Order Count: %d", frameInfo->pictureOrderCount);
            AVD_LOG_INFO("      Top Field Order Count: %d", frameInfo->topFieldOrderCount);
            AVD_LOG_INFO("      Bottom Field Order Count: %d", frameInfo->bottomFieldOrderCount);
            AVD_LOG_INFO("      Complementary Field Pair: %s", frameInfo->complementaryFieldPair ? "Yes" : "No");
        }
    }
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

bool avdH264VideoLoadChunk(AVD_H264Video *video, AVD_H264VideoChunk **outChunk, bool *eof)
{
    AVD_ASSERT(video != NULL);

    AVD_CHECK(__avdH264VideoResetChunk(video));

    picoH264NALUnitHeader_t nalUnitHeader = {0};
    bool idrEncountered                   = false;
    bool spsDirty                         = false;
    bool ppsDirty                         = false;

    do {
        AVD_Size currentCursor = video->bitstream->tell(video->bitstream->userData);
        if (!__avdH264VideoPeekNextNalUnit(video, &nalUnitHeader, true, eof)) {
            if (eof && *eof) {
                break;
            }
            AVD_CHECK_MSG(false, "Failed to peek next NAL unit");
        }
        if (nalUnitHeader.nalUnitType == PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_IDR) {
            if (idrEncountered) {
                break;
            }
            idrEncountered = true;
        } else if (nalUnitHeader.nalUnitType == PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_NON_IDR && !idrEncountered) {
            // if we havent yet encountered an IDR frame, we skip non-IDR frames
            AVD_Size nalUnitSize = 0;
            if (!picoH264FindNextNALUnit(video->bitstream, &nalUnitSize)) {
                if (eof) {
                    *eof = true;
                }
                break;
            }
            video->bitstream->seek(video->bitstream->userData, nalUnitSize, SEEK_CUR);
            continue;
        }
        if (!__avdH264VideoParseNextNalUnit(video, &nalUnitHeader, &spsDirty, &ppsDirty, eof)) {
            if (eof && *eof) {
                break;
            }
            AVD_CHECK_MSG(false, "Failed to parse next NAL unit");
        }
    } while (true);

    video->currentChunk.spsArray = &video->sps[0];
    video->currentChunk.spsHash  = video->spsHash;

    video->currentChunk.ppsArray = &video->pps[0];
    video->currentChunk.ppsHash  = video->ppsHash;

    video->currentChunk.timestampSeconds = 0.0;
    if (video->currentChunk.frameInfos.count > 0) {
        AVD_H264VideoFrameInfo *firstFrameInfo = (AVD_H264VideoFrameInfo *)avdListGet(&video->currentChunk.frameInfos, 0);
        video->currentChunk.timestampSeconds   = firstFrameInfo->timestampSeconds;
    }
    video->currentChunk.durationSeconds = video->frameDurationSeconds * (AVD_Float)video->currentChunk.frameInfos.count;

    *outChunk = &video->currentChunk;

    return true;
}