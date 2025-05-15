#ifndef PS_BLOOM_H
#define PS_BLOOM_H

#include "ps_common.h"

bool psBloomCreate(PS_Bloom *bloom, PS_Vulkan* vulkan, uint32_t width, uint32_t height); 
void psBloomDestroy(PS_Bloom *bloom, PS_Vulkan* vulkan);

#endif // PS_BLOOM_H