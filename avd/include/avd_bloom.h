#ifndef AVD_BLOOM_H
#define AVD_BLOOM_H

#include "avd_common.h"

bool avdBloomCreate(AVD_Bloom *bloom, AVD_Vulkan* vulkan, uint32_t width, uint32_t height); 
void avdBloomDestroy(AVD_Bloom *bloom, AVD_Vulkan* vulkan);

#endif // AVD_BLOOM_H