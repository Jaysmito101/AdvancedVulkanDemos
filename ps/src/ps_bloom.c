#include "ps_bloom.h"

bool psBloomCreate(PS_Bloom *bloom, PS_Vulkan* vulkan, uint32_t width, uint32_t height) 
{
    

}


void psBloomDestroy(PS_Bloom *bloom, PS_Vulkan* vulkan) 
{
    PS_ASSERT(bloom != NULL);
    PS_ASSERT(vulkan != NULL);

    // for (uint32_t i = 0; i < bloom->passCount; ++i) 
    // {
    //     psVulkanFramebufferDestroy(vulkan, &bloom->bloomPasses[i]);
    // }

}
