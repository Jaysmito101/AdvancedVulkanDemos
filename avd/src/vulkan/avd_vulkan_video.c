#include "vulkan/avd_vulkan_video.h"
#include "math/avd_math_base.h"

static bool __avdVulkanVideoCreateSession(AVD_Vulkan *vulkan, AVD_VulkanVideo *video)
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
        .maxActiveReferencePictures = AVD_MIN(vulkan->supportedFeatures.videoCapabilities.maxActiveReferencePictures, 16),
        .maxDpbSlots                = AVD_MIN(vulkan->supportedFeatures.videoCapabilities.maxDpbSlots, 17),
        .maxCodedExtent             = (VkExtent2D){
                        .width  = AVD_MIN(vulkan->supportedFeatures.videoCapabilities.maxCodedExtent.width, AVD_VULKAN_VIDEO_MAX_WIDTH),
                        .height = AVD_MIN(vulkan->supportedFeatures.videoCapabilities.maxCodedExtent.height, AVD_VULKAN_VIDEO_MAX_HEIGHT),
        },
        .pictureFormat          = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, // NOTE: as for now there is no need to support any other formats
        .referencePictureFormat = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        .pVideoProfile          = &videoProfileInfo,
        .pStdHeaderVersion      = &vulkan->supportedFeatures.videoCapabilities.stdHeaderVersion,
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
    }
    video->memoryAllocationCount = memoryRequirementsCount;

    AVD_CHECK_VK_RESULT(vkBindVideoSessionMemoryKHR(vulkan->device, video->session, memoryRequirementsCount, bindMemoryInfo), "Failed to bind video session memory");

    return true;
}

bool avdVulkanVideoCreate(AVD_Vulkan *vulkan, AVD_VulkanVideo *video)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(video != NULL);

    if (!vulkan->supportedFeatures.videoDecode) {
        AVD_LOG_ERROR("Vulkan video decode not supported on this device");
        return false;
    }

    AVD_CHECK(__avdVulkanVideoCreateSession(vulkan, video));

    return true;
}

void avdVulkanVideoDestroy(AVD_Vulkan *vulkan, AVD_VulkanVideo *video)
{
    vkDestroyVideoSessionKHR(vulkan->device, video->session, NULL);
    for (AVD_UInt32 i = 0; i < video->memoryAllocationCount; ++i) {
        vkFreeMemory(vulkan->device, video->memory[i], NULL);
    }
}