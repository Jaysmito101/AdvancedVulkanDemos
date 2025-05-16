#include "avd_bloom.h"

bool avdBloomCreate(AVD_Bloom *bloom, AVD_Vulkan* vulkan, uint32_t width, uint32_t height) 
{
    
    return true;
}


void avdBloomDestroy(AVD_Bloom *bloom, AVD_Vulkan* vulkan) 
{
    AVD_ASSERT(bloom != NULL);
    AVD_ASSERT(vulkan != NULL);

    // for (uint32_t i = 0; i < bloom->passCount; ++i) 
    // {
    //     avdVulkanFramebufferDestroy(vulkan, &bloom->bloomPasses[i]);
    // }

}
