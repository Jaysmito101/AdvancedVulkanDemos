#ifndef AVD_BLOOM_H
#define AVD_BLOOM_H

#include "core/avd_core.h"
#include "vulkan/avd_vulkan.h"

#ifndef AVD_BLOOM_PASS_COUNT
#define AVD_BLOOM_PASS_COUNT 5
#endif

typedef enum AVD_BloomPassType {
    AVD_BLOOM_PASS_TYPE_PREFILTER                    = 0,
    AVD_BLOOM_PASS_TYPE_DOWNSAMPLE                   = 1,
    AVD_BLOOM_PASS_TYPE_UPSAMPLE                     = 2,
    AVD_BLOOM_PASS_TYPE_COMPOSITE                    = 3,
    AVD_BLOOM_PASS_TYPE_DOWNSAMPLE_PREFILTER         = 4,
    AVD_BLOOM_PASS_TYPE_COUNT
} AVD_BloomPassType;

typedef enum AVD_BloomPrefilterType {
    AVD_BLOOM_PREFILTER_TYPE_NONE      = 0,
    AVD_BLOOM_PREFILTER_TYPE_THRESHOLD = 1,
    AVD_BLOOM_PREFILTER_TYPE_SOFTKNEE  = 2,
    AVD_BLOOM_PREFILTER_TYPE_COUNT
} AVD_BloomPrefilterType;

typedef enum AVD_BloomTonemappingType {
    AVD_BLOOM_TONEMAPPING_TYPE_NONE    = 0,
    AVD_BLOOM_TONEMAPPING_TYPE_ACES    = 1,
    AVD_BLOOM_TONEMAPPING_TYPE_FILMIC  = 2,
    AVD_BLOOM_TONEMAPPING_TYPE_COUNT
} AVD_BloomTonemappingType;

typedef struct AVD_BloomParams {
    AVD_BloomPrefilterType prefilterType;
    float threshold;
    float softKnee;
    float bloomAmount;
    bool lowQuality;
    bool applyGamma;
    AVD_BloomTonemappingType tonemappingType;
} AVD_BloomParams;

typedef struct AVD_Bloom {
    // NOTE: Ideally we dont really need 10 buffers for a 5 pass bloom effect, but this is a
    // simple way to do it for now. We can optimize this later if needed.
    // We can only do it using the first 5 buffers to first do the downsampling passes
    // then go back AVD_BLOOM_PASS_COUNT -> 1 to do the upsampling passes with the same buffers
    // and some blending.
    // This is a waste of memory, but it is simple to implement for now.
    AVD_VulkanFramebuffer bloomPasses[AVD_BLOOM_PASS_COUNT * 2 - 1];
    uint32_t passCount;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    // We need a seperate pipeline for compositing as the
    // framebuffer might not be compatible with the bloom pass framebuffer
    VkPipeline pipelineComposite;

    VkDescriptorSetLayout bloomDescriptorSetLayout;

    uint32_t width;
    uint32_t height;
} AVD_Bloom;

const char *avdBloomPassTypeToString(AVD_BloomPassType type);
const char *avdBloomPrefilterTypeToString(AVD_BloomPrefilterType type);
bool avdBloomCreate(AVD_Bloom *bloom, AVD_Vulkan *vulkan, VkRenderPass compositeRenderPass, uint32_t width, uint32_t height);
void avdBloomDestroy(AVD_Bloom *bloom, AVD_Vulkan *vulkan);

bool avdBloomApplyInplace(
    VkCommandBuffer commandBuffer,
    AVD_Bloom *bloom,
    AVD_VulkanFramebuffer *inputFramebuffer,
    AVD_Vulkan *vulkan,
    AVD_BloomParams params
);

#endif // AVD_BLOOM_H