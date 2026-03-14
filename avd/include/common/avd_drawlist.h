#ifndef AVD_DRAWLIST_H
#define AVD_DRAWLIST_H

#include "core/avd_core.h"
#include "core/avd_types.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "model/avd_model_base.h"
#include "vulkan/avd_vulkan.h"
#include "vulkan/avd_vulkan_buffer.h"

#ifndef AVD_DRAWLIST_MAX_TEXTURES
#define AVD_DRAWLIST_MAX_TEXTURES 1024
#endif

#ifndef AVD_DRAWLIST_MAX_CLIP_RECTS
#define AVD_DRAWLIST_MAX_CLIP_RECTS 1024
#endif

typedef struct {
    AVD_UInt32 vertexOffset;
    AVD_UInt32 vertexCount;
    void *textureHandle;
} AVD_DrawListCommand;

typedef struct {
    AVD_ModelVertexPacked *vertices;
    AVD_Size vertexCount;
    AVD_DrawListCommand *commands;
    AVD_Size commandCount;
    AVD_UInt32 width;
    AVD_UInt32 height;
} AVD_DrawListPacked;

typedef struct {
    AVD_Vector2 min;
    AVD_Vector2 max;
} AVD_DrawListClipRect;

typedef struct {
    AVD_List vertexData;
    AVD_List commands;

    void* textureStack[AVD_DRAWLIST_MAX_TEXTURES];
    int textureStackTop;
    
    AVD_DrawListClipRect clipRectStack[AVD_DRAWLIST_MAX_CLIP_RECTS];
    int clipRectStackTop;

    bool recording;

    AVD_UInt32 framebufferWidth;
    AVD_UInt32 framebufferHeight;

    AVD_DrawListCommand currentCommand;
    AVD_FontManager *fontManager; // for all the font glyph data and texture atlas offsets
} AVD_DrawList;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout bufferDescriptorSetLayout;
    VkDescriptorSetLayout textureDescriptorSetLayout;

    VkDescriptorSet bufferDescriptorSet;

    AVD_VulkanBuffer vertexBuffer;
} AVD_DrawListRenderer;

bool avdDrawListCreate(AVD_DrawList *drawList, AVD_FontManager *fontManager);
void avdDrawListDestroy(AVD_DrawList *drawList);

bool avdDrawListPushClipRect(AVD_DrawList *drawList, AVD_Vector2 min, AVD_Vector2 max);
void avdDrawListPopClipRect(AVD_DrawList *drawList);

bool avdDrawListPushTexture(AVD_DrawList *drawList, void *textureHandle);
void avdDrawListPopTexture(AVD_DrawList *drawList);

bool avdDrawListBegin(AVD_DrawList *drawList, AVD_UInt32 width, AVD_UInt32 height);
void avdDrawListEnd(AVD_DrawList *drawList);

bool avdDrawListAddTriangle(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos, AVD_Vector2 v1UV,
    AVD_Vector2 v2Pos, AVD_Vector2 v2UV,
    AVD_Vector2 v3Pos, AVD_Vector2 v3UV,
    AVD_Vector3 color);

bool avdDrawListAddQuadUv(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos, AVD_Vector2 v1UV,
    AVD_Vector2 v2Pos, AVD_Vector2 v2UV,
    AVD_Vector2 v3Pos, AVD_Vector2 v3UV,
    AVD_Vector2 v4Pos, AVD_Vector2 v4UV,
    AVD_Vector3 color);
bool avdDrawListAddQuad(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos,
    AVD_Vector2 v2Pos,
    AVD_Vector2 v3Pos,
    AVD_Vector2 v4Pos,
    AVD_Vector3 color);

bool avdDrawListAddRectUv(
    AVD_DrawList *drawList,
    AVD_Vector2 minPos, AVD_Vector2 maxPos,
    AVD_Vector2 uvMin, AVD_Vector2 uvMax,
    AVD_Vector3 color);
bool avdDrawListAddRect(
    AVD_DrawList *drawList,
    AVD_Vector2 minPos, AVD_Vector2 maxPos,
    AVD_Vector3 color);

bool avdDrawListAddCircleUv(
    AVD_DrawList *drawList,
    AVD_Vector2 center, float radius,
    AVD_Vector2 uvCenter, float uvRadius,
    AVD_Vector3 color,
    int segmentCount);
bool avdDrawListAddCircle(
    AVD_DrawList *drawList,
    AVD_Vector2 center, float radius,
    AVD_Vector3 color,
    int segmentCount);

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
