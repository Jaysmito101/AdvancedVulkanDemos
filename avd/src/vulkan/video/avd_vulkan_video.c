#include "vulkan/video/avd_vulkan_video_core.h"

const VkVideoProfileInfoKHR *avdVulkanVideoGetH264DecodeProfileInfo()
{
    static VkVideoDecodeH264ProfileInfoKHR h264DecodeProfileInfo = {
        .sType         = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR,
        .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH,
        .pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR,
        .pNext         = NULL,
    };

    static VkVideoProfileInfoKHR videoProfileInfo = {
        .sType               = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
        .videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR,
        .lumaBitDepth        = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
        .chromaBitDepth      = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
        .chromaSubsampling   = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
        .pNext               = &h264DecodeProfileInfo,
    };

    return &videoProfileInfo;
}

const VkVideoProfileInfoKHR *avdVulkanVideoGetH264EncodeProfileInfo()
{
    static VkVideoEncodeH264ProfileInfoKHR h264EncodeProfileInfo = {
        .sType         = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR,
        .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH,
        .pNext         = NULL,
    };

    static VkVideoProfileInfoKHR videoProfileInfo = {
        .sType               = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
        .videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR,
        .lumaBitDepth        = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
        .chromaBitDepth      = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
        .chromaSubsampling   = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
        .pNext               = &h264EncodeProfileInfo,
    };

    return &videoProfileInfo;
}

const VkVideoProfileInfoKHR *avdVulkanVideoGetH264ProfileInfo(AVD_Bool forDecode)
{
    if (forDecode) {
        return avdVulkanVideoGetH264DecodeProfileInfo();
    } else {
        return avdVulkanVideoGetH264EncodeProfileInfo();
    }
}

const VkVideoProfileListInfoKHR *avdVulkanVideoGetH264DecodeProfileListInfo()
{
    static VkVideoProfileListInfoKHR videoProfileListInfo = {
        .sType        = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR,
        .pProfiles    = NULL,
        .profileCount = 1,
        .pNext        = NULL,
    };
    videoProfileListInfo.pProfiles = avdVulkanVideoGetH264DecodeProfileInfo();

    return &videoProfileListInfo;
}

const VkVideoProfileListInfoKHR *avdVulkanVideoGetH264EncodeProfileListInfo()
{
    static VkVideoProfileListInfoKHR videoProfileListInfo = {
        .sType        = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR,
        .pProfiles    = NULL,
        .profileCount = 1,
        .pNext        = NULL,
    };
    videoProfileListInfo.pProfiles = avdVulkanVideoGetH264EncodeProfileInfo();

    return &videoProfileListInfo;
}

const VkVideoProfileListInfoKHR *avdVulkanVideoGetH264ProfileListInfo(AVD_Bool forDecode)
{
    if (forDecode) {
        return avdVulkanVideoGetH264DecodeProfileListInfo();
    } else {
        return avdVulkanVideoGetH264EncodeProfileListInfo();
    }
}
