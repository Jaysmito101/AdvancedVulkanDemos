#include "vulkan/video/avd_vulkan_video_decoder.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "math/avd_math_base.h"
#include "pico/picoH264.h"
#include "pico/picoPerf.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/avd_vulkan_image.h"
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
                video->h264Video->paddedWidth,
                video->h264Video->paddedHeight,
                dpbLabel));
        AVD_LOG_VERBOSE("Created DPB with %u slots for video decoder", video->dpb.numDPBSlots);
    }

    return true;
}

static bool __avdVulkanVideoDecoderUpdateDecodedFrames(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_VulkanVideoDecoderChunk *chunk)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(chunk != NULL);

    AVD_Size numFramesRecreated = 0;
    for (AVD_Size i = 0; i < AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES; i++) {
        AVD_VulkanVideoDecodedFrame *frame = &video->decodedFrames[i];
        if (!frame->initialized || frame->image.info.width != video->h264Video->width || frame->image.info.height != video->h264Video->height) {
            if (frame->initialized) {
                avdVulkanVideoDecodedFrameDestroy(vulkan, frame);
            }

            char frameLabel[64];
            snprintf(frameLabel, sizeof(frameLabel), "%s/DecodedFrame_%zu", video->label, i);

            AVD_CHECK(
                avdVulkanVideoDecodedFrameCreate(
                    vulkan,
                    frame,
                    video->h264Video->paddedWidth,
                    video->h264Video->paddedHeight,
                    video->dpb.format,
                    frameLabel));

            numFramesRecreated++;
        }
    }

    if (numFramesRecreated > 0) {
        AVD_LOG_VERBOSE("Recreated %zu decoded frames for video decoder", numFramesRecreated);
    }

    return true;
}

static bool __avdVulkanVideoDecoderPrepareVulkanPpsData(
    StdVideoH264PictureParameterSet *vPps,
    StdVideoH264ScalingLists *vScalingLists,
    picoH264PictureParameterSet pps)
{
    AVD_ASSERT(vPps != NULL);
    AVD_ASSERT(vScalingLists != NULL);
    AVD_ASSERT(pps != NULL);

    vPps->flags.transform_8x8_mode_flag                      = pps->transform8x8ModeFlag;
    vPps->flags.redundant_pic_cnt_present_flag               = pps->redundantPicCntPresentFlag;
    vPps->flags.constrained_intra_pred_flag                  = pps->constrainedIntraPredFlag;
    vPps->flags.deblocking_filter_control_present_flag       = pps->deblockingFilterControlPresentFlag;
    vPps->flags.weighted_pred_flag                           = pps->weightedPredFlag;
    vPps->flags.bottom_field_pic_order_in_frame_present_flag = pps->bottomFieldPicOrderInFramePresentFlag;
    vPps->flags.entropy_coding_mode_flag                     = pps->entropyCodingModeFlag;
    vPps->flags.pic_scaling_matrix_present_flag              = pps->picScalingMatrixPresentFlag;

    vPps->seq_parameter_set_id                 = pps->seqParameterSetId;
    vPps->pic_parameter_set_id                 = pps->picParameterSetId;
    vPps->num_ref_idx_l0_default_active_minus1 = pps->numRefIdxL0DefaultActiveMinus1;
    vPps->num_ref_idx_l1_default_active_minus1 = pps->numRefIdxL1DefaultActiveMinus1;
    vPps->weighted_bipred_idc                  = (StdVideoH264WeightedBipredIdc)pps->weightedBipredIdc;
    vPps->pic_init_qp_minus26                  = pps->picInitQpMinus26;
    vPps->pic_init_qs_minus26                  = pps->picInitQsMinus26;
    vPps->chroma_qp_index_offset               = pps->chromaQpIndexOffset;
    vPps->second_chroma_qp_index_offset        = pps->secondChromaQpIndexOffset;

    vPps->pScalingLists = vScalingLists;

    for (AVD_Size j = 0; j < AVD_ARRAY_COUNT(pps->picScalingListPresentFlag); ++j) {
        vScalingLists->scaling_list_present_mask |= pps->picScalingListPresentFlag[j] << j;
    }
    for (AVD_Size j = 0; j < AVD_ARRAY_COUNT(pps->useDefaultScalingMatrix4x4Flag); ++j) {
        vScalingLists->use_default_scaling_matrix_mask |= pps->useDefaultScalingMatrix4x4Flag[j] << j;
    }
    for (AVD_Size j = 0; j < AVD_ARRAY_COUNT(pps->scalingList4x4); ++j) {
        for (AVD_Size k = 0; k < AVD_ARRAY_COUNT(pps->scalingList4x4[j]); ++k) {
            vScalingLists->ScalingList4x4[j][k] = (uint8_t)pps->scalingList4x4[j][k];
        }
    }
    for (AVD_Size j = 0; j < AVD_ARRAY_COUNT(pps->scalingList8x8); ++j) {
        for (AVD_Size k = 0; k < AVD_ARRAY_COUNT(pps->scalingList8x8[j]); ++k) {
            vScalingLists->ScalingList8x8[j][k] = (uint8_t)pps->scalingList8x8[j][k];
        }
    }

    return true;
}

static bool __avdVulkanVideoDecoderLevelIdcToStdVideo(
    uint8_t levelIdc,
    StdVideoH264LevelIdc *outStdLevelIdc)
{
    AVD_ASSERT(outStdLevelIdc != NULL);
    switch (levelIdc) {
        case 0:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_1_0;
            break;
        case 11:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_1_1;
            break;
        case 12:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_1_2;
            break;
        case 13:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_1_3;
            break;
        case 20:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_2_0;
            break;
        case 21:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_2_1;
            break;
        case 22:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_2_2;
            break;
        case 30:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_3_0;
            break;
        case 31:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_3_1;
            break;
        case 32:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_3_2;
            break;
        case 40:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_4_0;
            break;
        case 41:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_4_1;
            break;
        case 42:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_4_2;
            break;
        case 50:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_5_0;
            break;
        case 51:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_5_1;
            break;
        case 52:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_5_2;
            break;
        case 60:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_6_0;
            break;
        case 61:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_6_1;
            break;
        case 62:
            *outStdLevelIdc = STD_VIDEO_H264_LEVEL_IDC_6_2;
            break;
        default:
            return false;
            break;
    }
    return true;
}

static bool __avdVulkanVideoDecoderPrepareVulkanSpsData(
    AVD_Vulkan *vulkan,
    StdVideoH264SequenceParameterSet *vSps,
    StdVideoH264SequenceParameterSetVui *vVui,
    StdVideoH264HrdParameters *vHrd,
    picoH264SequenceParameterSet sps)
{
    AVD_ASSERT(vSps != NULL);
    AVD_ASSERT(vVui != NULL);
    AVD_ASSERT(vHrd != NULL);
    AVD_ASSERT(sps != NULL);

    vSps->flags.constraint_set0_flag                 = sps->constraintSet0Flag;
    vSps->flags.constraint_set1_flag                 = sps->constraintSet1Flag;
    vSps->flags.constraint_set2_flag                 = sps->constraintSet2Flag;
    vSps->flags.constraint_set3_flag                 = sps->constraintSet3Flag;
    vSps->flags.constraint_set4_flag                 = sps->constraintSet4Flag;
    vSps->flags.constraint_set5_flag                 = sps->constraintSet5Flag;
    vSps->flags.direct_8x8_inference_flag            = sps->direct8x8InferenceFlag;
    vSps->flags.mb_adaptive_frame_field_flag         = sps->mbAdaptiveFrameFieldFlag;
    vSps->flags.frame_mbs_only_flag                  = sps->frameMbsOnlyFlag;
    vSps->flags.delta_pic_order_always_zero_flag     = sps->deltaPicOrderAlwaysZeroFlag;
    vSps->flags.separate_colour_plane_flag           = sps->seperateColourPlaneFlag;
    vSps->flags.gaps_in_frame_num_value_allowed_flag = sps->gapsInFrameNumValueAllowedFlag;
    vSps->flags.qpprime_y_zero_transform_bypass_flag = sps->qpprimeYZeroTransformBypassFlag;
    vSps->flags.frame_cropping_flag                  = sps->frameCroppingFlag;
    vSps->flags.seq_scaling_matrix_present_flag      = sps->seqScalingMatrixPresentFlag;
    vSps->flags.vui_parameters_present_flag          = (AVD_UInt32)sps->vuiParametersPresentFlag;

    if (vSps->flags.vui_parameters_present_flag) {

        vSps->pSequenceParameterSetVui              = vVui;
        vVui->flags.aspect_ratio_info_present_flag  = sps->vui.aspectRatioInfoPresentFlag;
        vVui->flags.overscan_info_present_flag      = sps->vui.overscanInfoPresentFlag;
        vVui->flags.overscan_appropriate_flag       = sps->vui.overscanAppropriateFlag;
        vVui->flags.video_signal_type_present_flag  = sps->vui.videoSignalTypePresentFlag;
        vVui->flags.video_full_range_flag           = sps->vui.videoFullRangeFlag;
        vVui->flags.color_description_present_flag  = sps->vui.colourDescriptionPresentFlag;
        vVui->flags.chroma_loc_info_present_flag    = sps->vui.chromaLocInfoPresentFlag;
        vVui->flags.timing_info_present_flag        = sps->vui.timingInfoPresentFlag;
        vVui->flags.fixed_frame_rate_flag           = sps->vui.fixedFrameRateFlag;
        vVui->flags.bitstream_restriction_flag      = sps->vui.bitstreamRestrictionFlag;
        vVui->flags.nal_hrd_parameters_present_flag = sps->vui.nalHrdParametersPresentFlag;
        vVui->flags.vcl_hrd_parameters_present_flag = sps->vui.vclHrdParametersPresentFlag;

        vVui->aspect_ratio_idc                    = (StdVideoH264AspectRatioIdc)sps->vui.aspectRatioIdc;
        vVui->sar_width                           = sps->vui.sarWidth;
        vVui->sar_height                          = sps->vui.sarHeight;
        vVui->video_format                        = sps->vui.videoFormat;
        vVui->colour_primaries                    = sps->vui.colourPrimaries;
        vVui->transfer_characteristics            = sps->vui.transferCharacteristics;
        vVui->matrix_coefficients                 = sps->vui.matrixCoefficients;
        vVui->num_units_in_tick                   = sps->vui.numUnitsInTick;
        vVui->time_scale                          = sps->vui.timeScale;
        vVui->max_num_reorder_frames              = sps->vui.numReorderFrames;
        vVui->max_dec_frame_buffering             = sps->vui.maxDecFrameBuffering;
        vVui->chroma_sample_loc_type_top_field    = sps->vui.chromaSampleLocTypeTopField;
        vVui->chroma_sample_loc_type_bottom_field = sps->vui.chromaSampleLocTypeBottomField;

        vVui->pHrdParameters = vHrd;
        vHrd->cpb_cnt_minus1 = sps->vui.nalHrdParameters.cpbCntMinus1;
        vHrd->bit_rate_scale = sps->vui.nalHrdParameters.bitRateScale;
        vHrd->cpb_size_scale = sps->vui.nalHrdParameters.cpbSizeScale;
        for (int j = 0; j < AVD_ARRAY_COUNT(vHrd->cpb_size_value_minus1); ++j) {
            vHrd->bit_rate_value_minus1[j] = sps->vui.nalHrdParameters.bitRateValueMinus1[j];
            vHrd->cpb_size_value_minus1[j] = sps->vui.nalHrdParameters.cpbSizeValueMinus1[j];
            vHrd->cbr_flag[j]              = sps->vui.nalHrdParameters.cbrFlag[j];
        }
        vHrd->initial_cpb_removal_delay_length_minus1 = sps->vui.nalHrdParameters.initialCpbRemovalDelayLengthMinus1;
        vHrd->cpb_removal_delay_length_minus1         = sps->vui.nalHrdParameters.cpbRemovalDelayLengthMinus1;
        vHrd->dpb_output_delay_length_minus1          = sps->vui.nalHrdParameters.dpbOutputDelayLengthMinus1;
        vHrd->time_offset_length                      = sps->vui.nalHrdParameters.timeOffsetLength;
    }

    vSps->profile_idc = (StdVideoH264ProfileIdc)sps->profileIdc;
    AVD_CHECK(__avdVulkanVideoDecoderLevelIdcToStdVideo(sps->levelIdc, &vSps->level_idc));
    AVD_CHECK_MSG(vSps->level_idc <= vulkan->supportedFeatures.videoDecodeH264Capabilities.maxLevelIdc, "Unsupported H264 level idc: %d", sps->levelIdc);
    AVD_CHECK_MSG(sps->chromaFormatIdc == STD_VIDEO_H264_CHROMA_FORMAT_IDC_420, "Only 4:2:0 chroma format is supported in Vulkan video decoding right now");
    // vSps->chroma_format_idc = (StdVideoH264ChromaFormatIdc)sps->chromaFormatIdc;
    vSps->chroma_format_idc                     = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420; // only one we support currently
    vSps->seq_parameter_set_id                  = sps->seqParameterSetId;
    vSps->bit_depth_luma_minus8                 = sps->bitDepthLumaMinus8;
    vSps->bit_depth_chroma_minus8               = sps->bitDepthChromaMinus8;
    vSps->log2_max_frame_num_minus4             = (AVD_UInt8)sps->log2MaxFrameNumMinus4;
    vSps->pic_order_cnt_type                    = (StdVideoH264PocType)sps->picOrderCntType;
    vSps->offset_for_non_ref_pic                = sps->offsetForNonRefPic;
    vSps->offset_for_top_to_bottom_field        = sps->offsetForTopToBottomField;
    vSps->log2_max_pic_order_cnt_lsb_minus4     = sps->log2MaxPicOrderCntLsbMinus4;
    vSps->num_ref_frames_in_pic_order_cnt_cycle = sps->numRefFramesInPicOrderCntCycle;
    vSps->max_num_ref_frames                    = sps->maxNumRefFrames;
    vSps->pic_width_in_mbs_minus1               = (AVD_UInt32)sps->picWidthInMbsMinus1;
    vSps->pic_height_in_map_units_minus1        = (AVD_UInt32)sps->picHeightInMapUnitsMinus1;
    vSps->frame_crop_left_offset                = (AVD_UInt32)sps->frameCropLeftOffset;
    vSps->frame_crop_right_offset               = (AVD_UInt32)sps->frameCropRightOffset;
    vSps->frame_crop_top_offset                 = (AVD_UInt32)sps->frameCropTopOffset;
    vSps->frame_crop_bottom_offset              = (AVD_UInt32)sps->frameCropBottomOffset;
    vSps->pOffsetForRefFrame                    = sps->offsetForRefFrame;

    return true;
}

// NOTE: here we are recreating the SPS for every change, which is not optimal
// a more optimal approach would be to just use an update call to update the changed SPS/PPS
// and keep the existing ones, but this is simpler to implement for now
static bool __avdVulkanVideoDecoderUpdateSessionParameters(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_VulkanVideoDecoderChunk *chunk, bool spsDirty, bool ppsDirty)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(chunk != NULL);

    if (video->sessionParameters == VK_NULL_HANDLE || spsDirty || ppsDirty) {
        if (video->sessionParameters != VK_NULL_HANDLE) {
            vkDestroyVideoSessionParametersKHR(vulkan->device, video->sessionParameters, NULL);
            video->sessionParameters = VK_NULL_HANDLE;
        }

        AVD_Size ppsCount = 0;
        AVD_Size spsCount = 0;

        for (AVD_Size i = 0; i < PICO_H264_MAX_PPS_COUNT; i++) {
            if (chunk->videoChunk->ppsArray[i] != NULL) {
                ppsCount++;
            }
        }

        for (AVD_Size i = 0; i < PICO_H264_MAX_SPS_COUNT; i++) {
            if (chunk->videoChunk->spsArray[i] != NULL) {
                spsCount++;
            }
        }

        AVD_CHECK(
            avdVulkanVideoDecoderSessionDataEnsureSize(
                &video->sessionData,
                ppsCount,
                spsCount));

        AVD_Size index = 0;
        for (AVD_Size i = 0; i < PICO_H264_MAX_PPS_COUNT; i++) {
            if (chunk->videoChunk->ppsArray[i] != NULL) {
                AVD_CHECK(
                    __avdVulkanVideoDecoderPrepareVulkanPpsData(
                        &video->sessionData.ppsArray[index],
                        &video->sessionData.scalingListsArray[index],
                        chunk->videoChunk->ppsArray[i]));

                index++;
            }
        }

        index = 0;
        for (AVD_Size i = 0; i < PICO_H264_MAX_SPS_COUNT; i++) {
            if (chunk->videoChunk->spsArray[i] != NULL) {
                AVD_CHECK(
                    __avdVulkanVideoDecoderPrepareVulkanSpsData(
                        vulkan,
                        &video->sessionData.spsArray[index],
                        &video->sessionData.vuiArray[index],
                        &video->sessionData.hrdArray[index],
                        chunk->videoChunk->spsArray[i]));

                index++;
            }
        }

        VkVideoDecodeH264SessionParametersAddInfoKHR sessionParametersAddInfo = {
            .sType       = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR,
            .stdPPSCount = (AVD_UInt32)ppsCount,
            .pStdPPSs    = video->sessionData.ppsArray,
            .stdSPSCount = (AVD_UInt32)spsCount,
            .pStdSPSs    = video->sessionData.spsArray,
            .pNext       = NULL,
        };

        VkVideoDecodeH264SessionParametersCreateInfoKHR sessionParametersInfoH264 = {
            .sType              = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR,
            .maxStdPPSCount     = (AVD_UInt32)ppsCount,
            .maxStdSPSCount     = (AVD_UInt32)spsCount,
            .pParametersAddInfo = &sessionParametersAddInfo,
            .pNext              = NULL,
        };

        VkVideoSessionParametersCreateInfoKHR sessionParametersCreateInfo = {
            .sType                          = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR,
            .videoSession                   = video->session,
            .videoSessionParametersTemplate = VK_NULL_HANDLE,
            .pNext                          = &sessionParametersInfoH264,
        };
        AVD_CHECK_VK_RESULT(
            vkCreateVideoSessionParametersKHR(
                vulkan->device,
                &sessionParametersCreateInfo,
                NULL,
                &video->sessionParameters),
            "Failed to create video session parameters");

        AVD_LOG_VERBOSE("Created new session parameters for video decoder");
    }

    return true;
}

static bool __avdVulkanVideoDecoderPrepareForNewChunk(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video, AVD_VulkanVideoDecoderChunk *chunk, bool spsDirty, bool ppsDirty)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(chunk != NULL);
    AVD_ASSERT(vulkan != NULL);

    // reset current slice index
    video->currentChunk.currentSliceIndex = 0;

    AVD_CHECK(__avdVulkanVideoDecoderUpdateChunkBitstreamBuffer(vulkan, video, chunk));
    AVD_CHECK(__avdVulkanVideoDecoderUpdateDPB(vulkan, video, chunk));
    AVD_CHECK(__avdVulkanVideoDecoderUpdateDecodedFrames(vulkan, video, chunk));
    AVD_CHECK(__avdVulkanVideoDecoderUpdateSessionParameters(vulkan, video, chunk, spsDirty, ppsDirty));

    chunk->ready = true;
    return true;
}

bool avdVulkanVideoDecoderSessionDataEnsureSize(
    AVD_VulkanVideoDecoderVideoSessionData *sessionData,
    AVD_Size ppsCount,
    AVD_Size spsCount)
{
    AVD_ASSERT(sessionData != NULL);

    if (sessionData->ppsCapacity < ppsCount) {
        if (sessionData->ppsArray) {
            AVD_FREE(sessionData->ppsArray);
        }
        if (sessionData->scalingListsArray) {
            AVD_FREE(sessionData->scalingListsArray);
        }

        sessionData->ppsArray          = (StdVideoH264PictureParameterSet *)AVD_MALLOC(sizeof(StdVideoH264PictureParameterSet) * ppsCount);
        sessionData->scalingListsArray = (StdVideoH264ScalingLists *)AVD_MALLOC(sizeof(StdVideoH264ScalingLists) * ppsCount);
        AVD_CHECK_MSG(sessionData->ppsArray != NULL && sessionData->scalingListsArray != NULL, "Failed to allocate memory for PPS session data");

        sessionData->ppsCapacity = ppsCount;
    }

    if (sessionData->spsCapacity < spsCount) {
        if (sessionData->spsArray) {
            AVD_FREE(sessionData->spsArray);
        }
        if (sessionData->vuiArray) {
            AVD_FREE(sessionData->vuiArray);
        }
        if (sessionData->hrdArray) {
            AVD_FREE(sessionData->hrdArray);
        }

        sessionData->spsArray = (StdVideoH264SequenceParameterSet *)AVD_MALLOC(sizeof(StdVideoH264SequenceParameterSet) * spsCount);
        sessionData->vuiArray = (StdVideoH264SequenceParameterSetVui *)AVD_MALLOC(sizeof(StdVideoH264SequenceParameterSetVui) * spsCount);
        sessionData->hrdArray = (StdVideoH264HrdParameters *)AVD_MALLOC(sizeof(StdVideoH264HrdParameters) * spsCount);
        AVD_CHECK_MSG(sessionData->spsArray != NULL && sessionData->vuiArray != NULL && sessionData->hrdArray != NULL, "Failed to allocate memory for SPS session data");

        sessionData->spsCapacity = spsCount;
    }

    // memset 0 for all
    memset(sessionData->ppsArray, 0, sizeof(StdVideoH264PictureParameterSet) * sessionData->ppsCapacity);
    memset(sessionData->scalingListsArray, 0, sizeof(StdVideoH264ScalingLists) * sessionData->ppsCapacity);
    memset(sessionData->spsArray, 0, sizeof(StdVideoH264SequenceParameterSet) * sessionData->spsCapacity);
    memset(sessionData->vuiArray, 0, sizeof(StdVideoH264SequenceParameterSetVui) * sessionData->spsCapacity);
    memset(sessionData->hrdArray, 0, sizeof(StdVideoH264HrdParameters) * sessionData->spsCapacity);

    return true;
}

void avdVulkanVideoDecoderSessionDataDestroy(AVD_VulkanVideoDecoderVideoSessionData *sessionData)
{
    AVD_ASSERT(sessionData != NULL);

    AVD_FREE(sessionData->ppsArray);
    AVD_FREE(sessionData->scalingListsArray);

    AVD_FREE(sessionData->spsArray);
    AVD_FREE(sessionData->vuiArray);
    AVD_FREE(sessionData->hrdArray);

    memset(sessionData, 0, sizeof(AVD_VulkanVideoDecoderVideoSessionData));
}

bool avdVulkanVideoDecodedFrameCreate(
    AVD_Vulkan *vulkan,
    AVD_VulkanVideoDecodedFrame *frame,
    AVD_UInt32 width,
    AVD_UInt32 height,
    VkFormat format,
    const char *label)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(frame != NULL);
    AVD_ASSERT(!frame->initialized);

    memset(frame, 0, sizeof(AVD_VulkanVideoDecodedFrame));

    AVD_VulkanImageCreateInfo createInfo = {
        .width                          = width,
        .height                         = height,
        .format                         = format,
        .usage                          = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags                          = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        .depth                          = 1,
        .arrayLayers                    = 1,
        .mipLevels                      = 1,
        .samplerFilter                  = VK_FILTER_LINEAR,
        .samplerAddressMode             = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .skipDefaultSubresourceCreation = true,
    };

    AVD_CHECK(
        avdVulkanImageCreate(
            vulkan,
            &frame->image,
            createInfo));

    AVD_CHECK(
        avdVulkanImageYCbCrSubresourceCreate(
            vulkan,
            &frame->image,
            (VkImageSubresourceRange){
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            true,
            &frame->ycbcrSubresource));

    frame->initialized = true;

    return true;
}

void avdVulkanVideoDecodedFrameDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideoDecodedFrame *frame)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(frame != NULL);

    if (!frame->initialized) {
        return;
    }

    avdVulkanImageYCbCrSubresourceDestroy(vulkan, &frame->ycbcrSubresource);
    avdVulkanImageDestroy(vulkan, &frame->image);

    memset(frame, 0, sizeof(AVD_VulkanVideoDecodedFrame));
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

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT, // create the fence in signaled state
    };
    AVD_CHECK_VK_RESULT(
        vkCreateFence(vulkan->device, &fenceInfo, NULL, &video->decodeFence),
        "Failed to create decode fence");

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

    avdVulkanWaitIdle(vulkan);

    if (video->decodeFence != VK_NULL_HANDLE) {
        vkDestroyFence(vulkan->device, video->decodeFence, NULL);
        video->decodeFence = VK_NULL_HANDLE;
    }

    avdVulkanVideoDecoderSessionDataDestroy(&video->sessionData);

    for (AVD_Size i = 0; i < AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES; i++) {
        avdVulkanVideoDecodedFrameDestroy(vulkan, &video->decodedFrames[i]);
    }
    avdVulkanBufferDestroy(vulkan, &video->bitstreamBuffer);
    avdVulkanVideoDPBDestroy(vulkan, &video->dpb);

    if (video->sessionParameters != VK_NULL_HANDLE) {
        vkDestroyVideoSessionParametersKHR(vulkan->device, video->sessionParameters, NULL);
    }
    if (video->session != VK_NULL_HANDLE) {
        vkDestroyVideoSessionKHR(vulkan->device, video->session, NULL);
    }
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

    bool spsDirty = false;
    bool ppsDirty = false;

    AVD_CHECK(
        avdH264VideoLoadChunk(
            video->h264Video,
            chunkLoadParams,
            &video->currentChunk.videoChunk,
            &spsDirty,
            &ppsDirty,
            eof));

    AVD_CHECK(__avdVulkanVideoDecoderPrepareForNewChunk(vulkan, video, &video->currentChunk, spsDirty, ppsDirty));

    video->displayOrderOffset += video->currentChunk.videoChunk->frameInfos.count;
    video->timestampSecondsOffset += video->currentChunk.videoChunk->durationSeconds;

    return true;
}

bool avdVulkanVideoDecoderUpdate(AVD_Vulkan *vulkan, AVD_VulkanVideoDecoder *video)
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
        video->decodedFrames[0].inUse = true;
        video->currentChunk.currentSliceIndex++;
    }

    return true;
}

bool avdVulkanVideoDecoderAcquireDecodedFrame(
    AVD_VulkanVideoDecoder *video,
    AVD_Float currentTime,
    AVD_VulkanVideoDecodedFrame **outFrame)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(outFrame != NULL);

    *outFrame = NULL;

    // try to find a decoded frame that is inuse (decoded) but not acquired yet and
    // whose time range is matching the current time
    for (AVD_Size i = 0; i < AVD_VULKAN_VIDEO_MAX_DECODED_FRAMES; i++) {
        AVD_VulkanVideoDecodedFrame *frame = &video->decodedFrames[i];
        if (frame->inUse && !frame->isAcquired) {
            AVD_Float frameStartTime = frame->timestampSeconds;
            AVD_Float frameEndTime   = frame->timestampSeconds + video->h264Video->frameDurationSeconds;
            if (currentTime >= frameStartTime && currentTime < frameEndTime) {
                frame->isAcquired = true;
                *outFrame         = frame;
                return true;
            }
        }
    }

    return true;
}

bool avdVulkanVideoDecoderReleaseDecodedFrame(AVD_VulkanVideoDecoder *video, AVD_VulkanVideoDecodedFrame *frame)
{
    AVD_ASSERT(video != NULL);
    AVD_ASSERT(frame != NULL);

    // NOTE: Some addition book-keeping might be needed here in the future
    // thus this oneliner function exists for now... :)
    frame->isAcquired = false;

    return true;
}