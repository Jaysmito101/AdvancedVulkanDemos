#ifndef AVD_DRAWLIST_H
#define AVD_DRAWLIST_H

#include "core/avd_core.h"
#include "font/avd_font_renderer.h"
#include "vulkan/avd_vulkan.h"

typedef struct {

} AVD_DrawList;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
} AVD_DrawListRenderer;

bool avdDrawListCreate(AVD_DrawList *drawList);
void avdDrawListDestroy(AVD_DrawList *drawList);

bool avdDrawListPushClipRect(AVD_DrawList *drawList, float x, float y, float width, float height);
void avdDrawListPopClipRect(AVD_DrawList *drawList);

bool avdDrawListPushTexture(AVD_DrawList *drawList, void *textureHandle);
void avdDrawListPopTexture(AVD_DrawList *drawList);

bool avdDrawListBegin(AVD_DrawList *drawList);
void avdDrawListEnd(AVD_DrawList *drawList);

bool avdDrawListAddTriangle(
    AVD_DrawList *drawList,
    AVD_Float x1, AVD_Float y1, AVD_Float uv1x, AVD_Float uv1y,
    AVD_Float x2, AVD_Float y2, AVD_Float uv2x, AVD_Float uv2y,
    AVD_Float x3, AVD_Float y3, AVD_Float uv3x, AVD_Float uv3y,
    AVD_UInt32 color);


AVD_Bool avdDrawListRendererCreate(AVD_DrawListRenderer *renderer, AVD_VulkanFramebuffer *framebuffer, AVD_Vulkan *vulkan);
void avdDrawListRendererDestroy(AVD_DrawListRenderer *renderer, AVD_Vulkan *vulkan);
AVD_Bool avdDrawListRendererRender(AVD_DrawListRenderer *renderer, VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet);

#endif // AVD_DRAWLIST_H
