#ifndef PS_FONT_RENDERER_H
#define PS_FONT_RENDERER_H

#include "ps_common.h"
#include "ps_font.h"
#include "ps_vulkan.h"

typedef struct PS_Font {
    PS_FontData fontData;
    PS_VulkanImage fontAtlasImage;
    PS_VulkanBuffer fontAtlasBuffer;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
} PS_Font;


typedef struct PS_FontRenderer {
    PS_Font* fonts[1024];
    size_t fontCount;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;

    PS_VulkanImage fontAtlasImage;
} PS_FontRenderer;


#endif // PS_FONT_RENDERER_H