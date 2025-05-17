#ifndef AVD_BLOOM_H
#define AVD_BLOOM_H

#include "core/avd_core.h"
#include "vulkan/avd_vulkan.h"


typedef struct AVD_Bloom {
    AVD_VulkanFramebuffer bloomPasses[5];
    uint32_t passCount;

    uint32_t width;
    uint32_t height;
} AVD_Bloom;


bool avdBloomCreate(AVD_Bloom *bloom, AVD_Vulkan* vulkan, uint32_t width, uint32_t height); 
void avdBloomDestroy(AVD_Bloom *bloom, AVD_Vulkan* vulkan);

#endif // AVD_BLOOM_H