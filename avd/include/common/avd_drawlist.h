#ifndef AVD_DRAWLIST_H
#define AVD_DRAWLIST_H

#include "core/avd_core.h"
#include "font/avd_font_renderer.h"
#include "model/avd_model_base.h"
#include "vulkan/avd_vulkan.h"
#include "vulkan/avd_vulkan_buffer.h"

typedef struct {
    AVD_UInt32 vertexOffset;
    AVD_UInt32 indexOffset;
    AVD_UInt32 indexCount;
    void *textureHandle;
} AVD_DrawListCommand;

typedef struct {
    AVD_ModelVertexPacked *vertices;
    AVD_Size vertexCount;
    AVD_UInt32 *indices;
    AVD_Size indexCount;
    AVD_DrawListCommand *commands;
    AVD_Size commandCount;
} AVD_DrawListPacked;

typedef struct {
    AVD_List vertexData;
    AVD_List indexData;
    AVD_List commands;

} AVD_DrawList;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout bufferDescriptorSetLayout;
    VkDescriptorSetLayout textureDescriptorSetLayout;

    VkDescriptorSet bufferDescriptorSet;

    AVD_VulkanBuffer vertexBuffer;
    AVD_VulkanBuffer indexBuffer;
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

// NOTE: packedData doesnt own anything, its just a pointer to existing data owned by the drawlist,
// so the caller must ensure that the drawlist is not freed while the packeData is in use as well as
// the caller doesnt have the responsibility to free packedData
AVD_Bool avdDrawListPack(AVD_DrawList *drawList, AVD_DrawListPacked *packedData);

AVD_Bool avdDrawListRendererCreate(AVD_DrawListRenderer *renderer, AVD_VulkanFramebuffer *framebuffer, AVD_Vulkan *vulkan);
void avdDrawListRendererDestroy(AVD_DrawListRenderer *renderer, AVD_Vulkan *vulkan);
AVD_Bool avdDrawListRendererRender(
    AVD_DrawListRenderer *renderer,
    AVD_Vulkan *vulkan,
    VkCommandBuffer commandBuffer,
    AVD_DrawListPacked *drawListData);

#endif // AVD_DRAWLIST_H
