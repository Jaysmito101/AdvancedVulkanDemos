#include "common/avd_drawlist.h"
#include "core/avd_base.h"
#include "core/avd_list.h"
#include "core/avd_types.h"
#include "font/avd_font_renderer.h"
#include "geom/avd_2d_geom.h"
#include "math/avd_vector_non_simd.h"
#include "model/avd_model_base.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/avd_vulkan_framebuffer.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"

typedef struct AVD_DrawListPushConstants {
    AVD_UInt32 vertexOffset;
    AVD_UInt32 framebufferWidth;
    AVD_UInt32 framebufferHeight;
    AVD_Float fontPxRange;
} AVD_DrawListPushConstants;

typedef enum {
    AVD_DRAWLIST_CLIP_EDGE_LEFT,
    AVD_DRAWLIST_CLIP_EDGE_RIGHT,
    AVD_DRAWLIST_CLIP_EDGE_TOP,
    AVD_DRAWLIST_CLIP_EDGE_BOTTOM
} AVD_DrawListClipEdge;

static AVD_Bool PRIV_avdDrawListRendererSetupDescriptors(AVD_DrawListRenderer *renderer, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &renderer->textureDescriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        renderer->textureDescriptorSetLayout,
        "[DescriptorSetLayout][Common]:DrawList/Texture");

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &renderer->bufferDescriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 1,
        VK_SHADER_STAGE_VERTEX_BIT));
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        renderer->bufferDescriptorSetLayout,
        "[DescriptorSetLayout][Common]:DrawList/Buffer");

    return true;
}

static AVD_Bool PRIV_avdDrawListRendererResizeBuffersIfNeeded(AVD_DrawListRenderer *renderer, AVD_DrawListPacked *drawListData, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(drawListData != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_Size requiredVertexBufferSize = AVD_MAX(drawListData->vertexCount * sizeof(AVD_ModelVertexPacked), 4);

    if (renderer->vertexBuffer.size < requiredVertexBufferSize || !renderer->vertexBuffer.initialized) {
        if (renderer->vertexBuffer.initialized) {
            avdVulkanBufferDestroy(vulkan, &renderer->vertexBuffer);
        }
        AVD_CHECK(
            avdVulkanBufferCreate(
                vulkan,
                &renderer->vertexBuffer,
                requiredVertexBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                // NOTE: since we will be updating these buffers from the CPU every frame,
                // having them device local wont really help much, and it would require
                // an extra staging buffer for the uploads
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                "DrawList/VertexBuffer"));
        VkWriteDescriptorSet writeDescriptorSet = {0};
        avdWriteBufferDescriptorSet(&writeDescriptorSet, renderer->bufferDescriptorSet, 0, &renderer->vertexBuffer.descriptorBufferInfo);
        vkUpdateDescriptorSets(vulkan->device, 1, &writeDescriptorSet, 0, NULL);
    }
    return true;
}

static AVD_Bool PRIV_avdDrawListFinalizeCurrentCommand(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    drawList->currentCommand.vertexCount = (AVD_UInt32)(drawList->vertexData.count - drawList->currentCommand.vertexOffset);

    if (drawList->currentCommand.textureHandle == NULL) {
        // if we have not set a texture for this command, use the default font texture
        // so that we have something to bind
        AVD_Font *defaultFont = NULL;
        AVD_CHECK(avdFontManagerGetFont(drawList->fontManager, "Default", &defaultFont));
        drawList->currentCommand.textureHandle = (void *)defaultFont->fontDescriptorSet;
    }

    avdListPushBack(&drawList->commands, &drawList->currentCommand);
    drawList->currentCommand.vertexOffset = (AVD_UInt32)drawList->vertexData.count;
    drawList->currentCommand.vertexCount  = 0;

    return true;
}

static AVD_DrawListTexture *PRIV_avdDrawListGetCurrentTexture(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    if (drawList->textureStackTop >= 0) {
        return &drawList->textureStack[drawList->textureStackTop];
    }
    return NULL; // no texture
}

static AVD_DrawListClipRect *PRIV_avdDrawListGetCurrentClipRect(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    if (drawList->clipRectStackTop >= 0) {
        return &drawList->clipRectStack[drawList->clipRectStackTop];
    }
    return NULL; // no clip rect
}

static AVD_Float PRIV_avdDrawListGetClipDist(const AVD_ModelVertex *v, AVD_DrawListClipRect rect, AVD_DrawListClipEdge edge)
{
    switch (edge) {
        case AVD_DRAWLIST_CLIP_EDGE_LEFT:
            return v->position.x - rect.min.x;
        case AVD_DRAWLIST_CLIP_EDGE_RIGHT:
            return rect.max.x - v->position.x;
        case AVD_DRAWLIST_CLIP_EDGE_TOP:
            return v->position.y - rect.min.y;
        case AVD_DRAWLIST_CLIP_EDGE_BOTTOM:
            return rect.max.y - v->position.y;
    }
    return 0.0f;
}

static AVD_ModelVertex PRIV_avdDrawListInterpolateVertex(const AVD_ModelVertex *v1, const AVD_ModelVertex *v2, AVD_Float t)
{
    AVD_ModelVertex r = *v1;
    r.position.x      = v1->position.x + (v2->position.x - v1->position.x) * t;
    r.position.y      = v1->position.y + (v2->position.y - v1->position.y) * t;
    r.position.z      = v1->position.z + (v2->position.z - v1->position.z) * t;
    r.texCoord.x      = v1->texCoord.x + (v2->texCoord.x - v1->texCoord.x) * t;
    r.texCoord.y      = v1->texCoord.y + (v2->texCoord.y - v1->texCoord.y) * t;
    r.normal.x        = v1->normal.x + (v2->normal.x - v1->normal.x) * t;
    r.normal.y        = v1->normal.y + (v2->normal.y - v1->normal.y) * t;
    r.normal.z        = v1->normal.z + (v2->normal.z - v1->normal.z) * t;
    return r;
}

static AVD_Bool PRIV_avdDrawListClipTriangle(
    AVD_ModelVertex *vertices[3],
    AVD_DrawListClipRect clipRect,
    AVD_Size *clippedTriangleCount,
    AVD_ModelVertex clippedVertices[10][3])
{
    AVD_ModelVertex poly1[10];
    AVD_ModelVertex poly2[10];
    AVD_UInt32 polyCount = 3;

    poly1[0] = *vertices[0];
    poly1[1] = *vertices[1];
    poly1[2] = *vertices[2];

    for (int e = 0; e < 4; ++e) {
        AVD_UInt32 outCount       = 0;
        AVD_DrawListClipEdge edge = (AVD_DrawListClipEdge)e;

        for (int i = 0; i < polyCount; ++i) {
            int j                       = (i + 1) % polyCount;
            const AVD_ModelVertex *cur  = &poly1[i];
            const AVD_ModelVertex *next = &poly1[j];

            AVD_Float d1 = PRIV_avdDrawListGetClipDist(cur, clipRect, edge);
            AVD_Float d2 = PRIV_avdDrawListGetClipDist(next, clipRect, edge);

            if (d1 >= 0.0f && outCount < 10) {
                poly2[outCount++] = *cur;
            }
            if (((d1 >= 0.0f && d2 < 0.0f) || (d1 < 0.0f && d2 >= 0.0f)) && outCount < 10) {
                AVD_Float t       = d1 / (d1 - d2);
                poly2[outCount++] = PRIV_avdDrawListInterpolateVertex(cur, next, t);
            }
        }

        polyCount = outCount;
        for (int i = 0; i < polyCount; ++i) {
            poly1[i] = poly2[i];
        }

        if (polyCount == 0) {
            *clippedTriangleCount = 0;
            return true;
        }
    }

    *clippedTriangleCount = 0;
    for (int i = 1; i < polyCount - 1 && *clippedTriangleCount < 10; ++i) {
        clippedVertices[*clippedTriangleCount][0] = poly1[0];
        clippedVertices[*clippedTriangleCount][1] = poly1[i];
        clippedVertices[*clippedTriangleCount][2] = poly1[i + 1];
        (*clippedTriangleCount)++;
    }

    return true;
}

static AVD_Bool PRIV_avdDrawListAddTriangle(
    AVD_DrawList *drawList,
    AVD_ModelVertex vertices[3],
    void *textureHandle)
{
    AVD_ASSERT(drawList != NULL);

    AVD_CHECK_MSG(drawList->recording, "Must call avdDrawListBegin before adding draw commands");

    AVD_DrawListClipRect clipRect     = {0};
    AVD_DrawListClipRect *clipRectPtr = PRIV_avdDrawListGetCurrentClipRect(drawList);
    if (clipRectPtr != NULL) {
        clipRect = *clipRectPtr;
    } else {
        clipRect.min = avdVec2Zero();
        clipRect.max = avdVec2((AVD_Float)drawList->framebufferWidth, (AVD_Float)drawList->framebufferHeight);
    }

    AVD_GeomTriangleClipStatus clipStatus = avdGeomGetTriangleClipStatus(
        avdVec2(vertices[0].position.x, vertices[0].position.y),
        avdVec2(vertices[1].position.x, vertices[1].position.y),
        avdVec2(vertices[2].position.x, vertices[2].position.y),
        clipRect.min,
        clipRect.max);
    if (clipStatus == AVD_GEOM_TRIANGLE_CLIP_STATUS_OUTSIDE) {
        return true;
    }

    if (clipStatus == AVD_GEOM_TRIANGLE_CLIP_STATUS_CLIPPED) {
        AVD_ModelVertex *vPtrs[3] = {&vertices[0], &vertices[1], &vertices[2]};
        AVD_Size clippedCount     = 0;
        AVD_ModelVertex clipped[10][3];
        PRIV_avdDrawListClipTriangle(vPtrs, clipRect, &clippedCount, clipped);

        for (AVD_Size i = 0; i < clippedCount; ++i) {
            PRIV_avdDrawListAddTriangle(drawList, clipped[i], textureHandle);
        }
        return true;
    }

    if (textureHandle == NULL) {
        textureHandle = drawList->currentCommand.textureHandle;
    }

    if (drawList->currentCommand.textureHandle != textureHandle) {
        if (drawList->currentCommand.textureHandle == NULL) {
            drawList->currentCommand.textureHandle = textureHandle;
        } else {
            AVD_CHECK(PRIV_avdDrawListFinalizeCurrentCommand(drawList));
        }
    }

    AVD_ModelVertexPacked packedVertex = {0};
    for (int i = 0; i < 3; ++i) {
        avdModelVertexPack(&vertices[i], &packedVertex);
        avdListPushBack(&drawList->vertexData, &packedVertex);
    }

    drawList->currentCommand.textureHandle = textureHandle;

    return true;
}

AVD_Bool avdDrawListCreate(AVD_DrawList *drawList, AVD_FontManager *fontManager)
{
    AVD_ASSERT(drawList != NULL);

    memset(drawList, 0, sizeof(AVD_DrawList));

    drawList->fontManager      = fontManager;
    drawList->textureStackTop  = -1; // indicates an empty stack
    drawList->clipRectStackTop = -1; // indicates an empty stack
    drawList->recording        = false;

    avdListCreate(&drawList->commands, sizeof(AVD_DrawListCommand));
    avdListCreate(&drawList->vertexData, sizeof(AVD_ModelVertexPacked));

    return true;
}

void avdDrawListDestroy(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    avdListDestroy(&drawList->commands);
    avdListDestroy(&drawList->vertexData);

    memset(drawList, 0, sizeof(AVD_DrawList));
}

bool avdDrawListPushClipRect(AVD_DrawList *drawList, AVD_Vector2 min, AVD_Vector2 max)
{
    AVD_ASSERT(drawList != NULL);

    if (drawList->clipRectStackTop >= AVD_DRAWLIST_MAX_CLIP_RECTS - 1) {
        AVD_LOG_ERROR("DrawList clip rect stack overflow");
        return false;
    }

    drawList->clipRectStackTop++;
    drawList->clipRectStack[drawList->clipRectStackTop].min = min;
    drawList->clipRectStack[drawList->clipRectStackTop].max = max;

    return true;
}

void avdDrawListPopClipRect(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    if (drawList->clipRectStackTop < 0) {
        AVD_LOG_ERROR("DrawList clip rect stack underflow");
        return;
    }

    drawList->clipRectStackTop--;
}

bool avdDrawListPushTexture(AVD_DrawList *drawList, void *textureHandle)
{
    AVD_ASSERT(drawList != NULL);

    if (drawList->textureStackTop >= AVD_DRAWLIST_MAX_TEXTURES - 1) {
        AVD_LOG_ERROR("DrawList texture stack overflow");
        return false;
    }

    drawList->textureStackTop++;
    drawList->textureStack[drawList->textureStackTop] = (AVD_DrawListTexture){
        .handle        = textureHandle,
        .isFontTexture = false,
    };

    return true;
}

void avdDrawListPopTexture(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    if (drawList->textureStackTop < 0) {
        AVD_LOG_ERROR("DrawList texture stack underflow");
        return;
    }

    drawList->textureStackTop--;
}

bool avdDrawListBegin(AVD_DrawList *drawList, AVD_UInt32 width, AVD_UInt32 height)
{
    AVD_ASSERT(drawList != NULL);

    avdListClear(&drawList->vertexData);
    avdListClear(&drawList->commands);

    drawList->recording                    = true;
    drawList->framebufferWidth             = width;
    drawList->framebufferHeight            = height;
    drawList->currentCommand.vertexOffset  = 0;
    drawList->currentCommand.vertexCount   = 0;
    drawList->currentCommand.fontPxRange   = 0.0;
    drawList->currentCommand.textureHandle = NULL;
    drawList->textureStackTop              = -1;
    drawList->clipRectStackTop             = -1;

    return true;
}

void avdDrawListEnd(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    (void)PRIV_avdDrawListFinalizeCurrentCommand(drawList);

    drawList->recording = false;
}

AVD_Bool avdDrawListPack(AVD_DrawList *drawList, AVD_DrawListPacked *packedData)
{
    AVD_ASSERT(drawList != NULL);
    AVD_ASSERT(packedData != NULL);

    AVD_CHECK_MSG(!drawList->recording, "Must call avdDrawListEnd before packing the draw list data");

    memset(packedData, 0, sizeof(AVD_DrawListPacked));

    // NOTE: for now we dont necessarily need to pack as the data
    // as already being added in a packed format. However, later
    // we might implement layer merging optimizaitons where packing
    // will actually involved modifying the data.

    packedData->vertices     = drawList->vertexData.items;
    packedData->vertexCount  = drawList->vertexData.count;
    packedData->commands     = drawList->commands.items;
    packedData->commandCount = drawList->commands.count;
    packedData->width        = drawList->framebufferWidth;
    packedData->height       = drawList->framebufferHeight;
    return true;
}

bool avdDrawListAddTriangle(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos,
    AVD_Vector2 v2Pos,
    AVD_Vector2 v3Pos,
    AVD_Float thickness,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);

    AVD_CHECK(avdDrawListAddLine(drawList, v1Pos, v2Pos, thickness, color));
    AVD_CHECK(avdDrawListAddLine(drawList, v2Pos, v3Pos, thickness, color));
    AVD_CHECK(avdDrawListAddLine(drawList, v3Pos, v1Pos, thickness, color));

    return true;
}

AVD_Bool avdDrawListAddTriangleFilled(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos, AVD_Vector2 v1UV,
    AVD_Vector2 v2Pos, AVD_Vector2 v2UV,
    AVD_Vector2 v3Pos, AVD_Vector2 v3UV,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);
    AVD_CHECK_MSG(drawList->recording, "Must call avdDrawListBegin before adding draw commands");

    AVD_DrawListTexture *currentTexture = PRIV_avdDrawListGetCurrentTexture(drawList);

    AVD_ModelVertex vertices[3] = {
        {
            .position = {
                v1Pos.x,
                v1Pos.y,
                currentTexture != NULL ? currentTexture->isFontTexture ? 2.0f : 1.0f : 0.0f,
            },
            .normal   = {color.x, color.y, color.z},
            .texCoord = {v1UV.x, v1UV.y},
        },
        {
            .position = {
                v2Pos.x,
                v2Pos.y,
                currentTexture != NULL ? currentTexture->isFontTexture ? 2.0f : 1.0f : 0.0f,
            },
            .normal   = {color.x, color.y, color.z},
            .texCoord = {v2UV.x, v2UV.y},
        },
        {
            .position = {
                v3Pos.x,
                v3Pos.y,
                currentTexture != NULL ? currentTexture->isFontTexture ? 2.0f : 1.0f : 0.0f,
            },
            .normal   = {color.x, color.y, color.z},
            .texCoord = {v3UV.x, v3UV.y},
        },
    };

    AVD_CHECK(
        PRIV_avdDrawListAddTriangle(
            drawList,
            vertices,
            currentTexture ? currentTexture->handle : NULL));

    return true;
}

bool avdDrawListAddLine(
    AVD_DrawList *drawList,
    AVD_Vector2 startPos,
    AVD_Vector2 endPos,
    float thickness,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);
    AVD_CHECK_MSG(drawList->recording, "Must call avdDrawListBegin before adding draw commands");

    AVD_Vector2 dir     = avdVec2Normalize(avdVec2Subtract(endPos, startPos));
    AVD_Vector2 tangent = avdVec2(-dir.y, dir.x);
    AVD_Vector2 offset  = avdVec2Scale(tangent, thickness * 0.5f);

    AVD_Vector2 capOffset = avdVec2Scale(dir, thickness * 0.5f);
    AVD_Vector2 extStart  = avdVec2Subtract(startPos, capOffset);
    AVD_Vector2 extEnd    = avdVec2Add(endPos, capOffset);

    AVD_Vector2 v1Pos = avdVec2Subtract(extStart, offset);
    AVD_Vector2 v2Pos = avdVec2Subtract(extEnd, offset);
    AVD_Vector2 v3Pos = avdVec2Add(extEnd, offset);
    AVD_Vector2 v4Pos = avdVec2Add(extStart, offset);

    return avdDrawListAddQuadFilled(
        drawList,
        v1Pos,
        v2Pos,
        v3Pos,
        v4Pos,
        color);
}

AVD_Bool avdDrawListAddQuadFilledUv(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos, AVD_Vector2 v1UV,
    AVD_Vector2 v2Pos, AVD_Vector2 v2UV,
    AVD_Vector2 v3Pos, AVD_Vector2 v3UV,
    AVD_Vector2 v4Pos, AVD_Vector2 v4UV,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);

    AVD_CHECK(avdDrawListAddTriangleFilled(drawList, v1Pos, v1UV, v2Pos, v2UV, v3Pos, v3UV, color));
    AVD_CHECK(avdDrawListAddTriangleFilled(drawList, v1Pos, v1UV, v3Pos, v3UV, v4Pos, v4UV, color));
    return true;
}

bool avdDrawListAddQuadFilled(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos,
    AVD_Vector2 v2Pos,
    AVD_Vector2 v3Pos,
    AVD_Vector2 v4Pos,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);

    AVD_Vector2 uvMin = {0.0f, 0.0f};
    AVD_Vector2 uvMax = {1.0f, 1.0f};
    AVD_CHECK(avdDrawListAddQuadFilledUv(
        drawList,
        v1Pos,
        uvMin,
        v2Pos,
        (AVD_Vector2){uvMax.x, uvMin.y},
        v3Pos,
        uvMax,
        v4Pos,
        (AVD_Vector2){uvMin.x, uvMax.y},
        color));

    return true;
}

AVD_Bool avdDrawListAddRectFilledUv(
    AVD_DrawList *drawList,
    AVD_Vector2 minPos, AVD_Vector2 maxPos,
    AVD_Vector2 uvMin, AVD_Vector2 uvMax,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);

    AVD_Vector2 v1Pos = {minPos.x, minPos.y};
    AVD_Vector2 v2Pos = {maxPos.x, minPos.y};
    AVD_Vector2 v3Pos = {maxPos.x, maxPos.y};
    AVD_Vector2 v4Pos = {minPos.x, maxPos.y};

    AVD_Vector2 v1UV = {uvMin.x, uvMin.y};
    AVD_Vector2 v2UV = {uvMax.x, uvMin.y};
    AVD_Vector2 v3UV = {uvMax.x, uvMax.y};
    AVD_Vector2 v4UV = {uvMin.x, uvMax.y};

    AVD_CHECK(avdDrawListAddQuadFilledUv(drawList, v1Pos, v1UV, v2Pos, v2UV, v3Pos, v3UV, v4Pos, v4UV, color));
    return true;
}

bool avdDrawListAddRectFilled(
    AVD_DrawList *drawList,
    AVD_Vector2 minPos, AVD_Vector2 maxPos,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);

    AVD_Vector2 uvMin = {0.0f, 0.0f};
    AVD_Vector2 uvMax = {1.0f, 1.0f};
    AVD_CHECK(avdDrawListAddRectFilledUv(
        drawList,
        minPos,
        maxPos,
        uvMin,
        uvMax,
        color));

    return true;
}

AVD_Bool avdDrawListAddCircleFilledUv(
    AVD_DrawList *drawList,
    AVD_Vector2 center, float radius,
    AVD_Vector2 uvCenter, float uvRadius,
    AVD_Vector3 color,
    int segmentCount)
{
    AVD_ASSERT(drawList != NULL);

    segmentCount = AVD_CLAMP(segmentCount, 3, 360);

    float angleStep = 2.0f * AVD_PI / (float)segmentCount;

    AVD_Vector2 firstPos = {
        center.x + radius * cosf(0.0f),
        center.y + radius * sinf(0.0f)};
    AVD_Vector2 firstUV = {
        uvCenter.x + uvRadius * cosf(0.0f),
        uvCenter.y + uvRadius * sinf(0.0f)};

    AVD_Vector2 prevPos = firstPos;
    AVD_Vector2 prevUV  = firstUV;

    for (AVD_UInt32 i = 1; i <= (AVD_UInt32)segmentCount; i++) {
        float angle         = (AVD_Float)i * angleStep;
        AVD_Vector2 nextPos = {
            center.x + radius * cosf(angle),
            center.y + radius * sinf(angle)};
        AVD_Vector2 nextUV = {
            uvCenter.x + uvRadius * cosf(angle),
            uvCenter.y + uvRadius * sinf(angle)};

        if (i == segmentCount) {
            nextPos = firstPos;
            nextUV  = firstUV;
        }

        AVD_CHECK(avdDrawListAddTriangleFilled(drawList, center, uvCenter, prevPos, prevUV, nextPos, nextUV, color));

        prevPos = nextPos;
        prevUV  = nextUV;
    }
    return true;
}

bool avdDrawListAddCircleFilled(
    AVD_DrawList *drawList,
    AVD_Vector2 center, float radius,
    AVD_Vector3 color,
    int segmentCount)
{
    AVD_ASSERT(drawList != NULL);

    AVD_Vector2 uvCenter = {0.5f, 0.5f};
    float uvRadius       = 0.5f;
    return avdDrawListAddCircleFilledUv(
        drawList,
        center,
        radius,
        uvCenter,
        uvRadius,
        color,
        segmentCount);
}

bool avdDrawListAddQuad(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos,
    AVD_Vector2 v2Pos,
    AVD_Vector2 v3Pos,
    AVD_Vector2 v4Pos,
    AVD_Float thickness,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);
    AVD_CHECK(avdDrawListAddLine(drawList, v1Pos, v2Pos, thickness, color));
    AVD_CHECK(avdDrawListAddLine(drawList, v2Pos, v3Pos, thickness, color));
    AVD_CHECK(avdDrawListAddLine(drawList, v3Pos, v4Pos, thickness, color));
    AVD_CHECK(avdDrawListAddLine(drawList, v4Pos, v1Pos, thickness, color));

    return true;
}

bool avdDrawListAddRect(
    AVD_DrawList *drawList,
    AVD_Vector2 minPos, AVD_Vector2 maxPos,
    float thickness,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);

    AVD_Vector2 v1Pos = {minPos.x, minPos.y};
    AVD_Vector2 v2Pos = {maxPos.x, minPos.y};
    AVD_Vector2 v3Pos = {maxPos.x, maxPos.y};
    AVD_Vector2 v4Pos = {minPos.x, maxPos.y};

    AVD_CHECK(avdDrawListAddQuad(drawList, v1Pos, v2Pos, v3Pos, v4Pos, thickness, color));

    return true;
}

bool avdDrawListAddCircle(
    AVD_DrawList *drawList,
    AVD_Vector2 center, float radius,
    float thickness,
    AVD_Vector3 color,
    int segmentCount)
{
    AVD_ASSERT(drawList != NULL);

    segmentCount = AVD_CLAMP(segmentCount, 3, 360);

    float angleStep = 2.0f * AVD_PI / (float)segmentCount;

    AVD_Vector2 firstPos = {
        center.x + radius * cosf(0.0f),
        center.y + radius * sinf(0.0f)};

    AVD_Vector2 prevPos = firstPos;

    for (AVD_UInt32 i = 1; i <= (AVD_UInt32)segmentCount; i++) {
        float angle         = (AVD_Float)i * angleStep;
        AVD_Vector2 nextPos = {
            center.x + radius * cosf(angle),
            center.y + radius * sinf(angle)};

        if (i == segmentCount) {
            nextPos = firstPos;
        }

        AVD_CHECK(avdDrawListAddLine(drawList, prevPos, nextPos, thickness, color));

        prevPos = nextPos;
    }
    return true;
}

bool avdDrawListAddElipse(
    AVD_DrawList *drawList,
    AVD_Vector2 center, AVD_Vector2 radius,
    float thickness,
    AVD_Vector3 color,
    int segmentCount)
{
    AVD_ASSERT(drawList != NULL);

    segmentCount = AVD_CLAMP(segmentCount, 3, 360);

    float angleStep = 2.0f * AVD_PI / (float)segmentCount;

    AVD_Vector2 firstPos = {
        center.x + radius.x * cosf(0.0f),
        center.y + radius.y * sinf(0.0f)};

    AVD_Vector2 prevPos = firstPos;

    for (AVD_UInt32 i = 1; i <= (AVD_UInt32)segmentCount; i++) {
        float angle         = (AVD_Float)i * angleStep;
        AVD_Vector2 nextPos = {
            center.x + radius.x * cosf(angle),
            center.y + radius.y * sinf(angle)};

        if (i == segmentCount) {
            nextPos = firstPos;
        }

        AVD_CHECK(avdDrawListAddLine(drawList, prevPos, nextPos, thickness, color));

        prevPos = nextPos;
    }
    return true;
}

bool avdDrawListAddElipseFilledUv(
    AVD_DrawList *drawList,
    AVD_Vector2 center, AVD_Vector2 radius,
    AVD_Vector2 uvCenter, AVD_Vector2 uvRadius,
    AVD_Vector3 color,
    int segmentCount)
{
    AVD_ASSERT(drawList != NULL);

    segmentCount = AVD_CLAMP(segmentCount, 3, 360);

    float angleStep = 2.0f * AVD_PI / (float)segmentCount;

    AVD_Vector2 firstPos = {
        center.x + radius.x * cosf(0.0f),
        center.y + radius.y * sinf(0.0f)};
    AVD_Vector2 firstUV = {
        uvCenter.x + uvRadius.x * cosf(0.0f),
        uvCenter.y + uvRadius.y * sinf(0.0f)};

    AVD_Vector2 prevPos = firstPos;
    AVD_Vector2 prevUV  = firstUV;

    for (AVD_UInt32 i = 1; i <= (AVD_UInt32)segmentCount; i++) {
        float angle         = (AVD_Float)i * angleStep;
        AVD_Vector2 nextPos = {
            center.x + radius.x * cosf(angle),
            center.y + radius.y * sinf(angle)};
        AVD_Vector2 nextUV = {
            uvCenter.x + uvRadius.x * cosf(angle),
            uvCenter.y + uvRadius.y * sinf(angle)};

        if (i == segmentCount) {
            nextPos = firstPos;
            nextUV  = firstUV;
        }

        AVD_CHECK(avdDrawListAddTriangleFilled(drawList, center, uvCenter, prevPos, prevUV, nextPos, nextUV, color));

        prevPos = nextPos;
        prevUV  = nextUV;
    }
    return true;
}

bool avdDrawListAddElipseFilled(
    AVD_DrawList *drawList,
    AVD_Vector2 center, AVD_Vector2 radius,
    AVD_Vector3 color,
    int segmentCount)
{
    AVD_ASSERT(drawList != NULL);

    AVD_Vector2 uvCenter = {0.5f, 0.5f};
    AVD_Vector2 uvRadius = {0.5f, 0.5f};
    return avdDrawListAddElipseFilledUv(
        drawList,
        center,
        radius,
        uvCenter,
        uvRadius,
        color,
        segmentCount);
}

bool avdDrawListAddText(
    AVD_DrawList *drawList,
    const char *text,
    AVD_Vector2 position,
    float fontSize,
    AVD_Vector3 color,
    const char *fontName,
    AVD_Float wrapWidth)
{
    AVD_ASSERT(drawList != NULL);
    AVD_ASSERT(text != NULL);

    if (drawList->fontManager == NULL) {
        AVD_LOG_WARN("Cannot add text to draw list: fontManager is NULL");
        return false;
    }

    AVD_Font *font = NULL;
    if (!avdFontManagerGetFont(drawList->fontManager, fontName ? fontName : "Default", &font) || font == NULL) {
        AVD_LOG_WARN("Failed to get font: %s", fontName);
        return false;
    }

    // push a font texture for the text rendering
    drawList->textureStackTop++;
    drawList->textureStack[drawList->textureStackTop] = (AVD_DrawListTexture){
        .handle        = (void *)font->fontDescriptorSet,
        .isFontTexture = true,
    };
    drawList->currentCommand.fontPxRange = font->fontData.atlas->info.distanceRange;

    float lineHeight  = font->fontData.atlas->metrics.lineHeight;
    float atlasWidth  = (float)font->fontData.atlas->info.width;
    float atlasHeight = (float)font->fontData.atlas->info.height;

    float cX = position.x;
    float cY = position.y;

    for (size_t i = 0; i < strlen(text); i++) {
        uint32_t c = (uint32_t)text[i];
        if (c == '\n' || (wrapWidth > 0.0f && cX - position.x >= wrapWidth)) {
            cX = position.x;
            cY += fontSize * lineHeight;
            continue;
        }

        if (c >= AVD_FONT_MAX_GLYPHS) {
            continue;
        }

        if (c == ' ') {
            cX += font->fontData.atlas->glyphs[' '].advanceX * fontSize;
            continue;
        }

        if (c == '\r') {
            continue;
        }

        AVD_FontAtlasGlyph *glyph        = &font->fontData.atlas->glyphs[c];
        AVD_FontAtlasBounds *bounds      = &glyph->atlasBounds;
        AVD_FontAtlasBounds *planeBounds = &glyph->planeBounds;

        float ax = cX + planeBounds->left * fontSize;
        float ay = cY - planeBounds->top * fontSize;
        float bx = cX + planeBounds->right * fontSize;
        float by = cY - planeBounds->bottom * fontSize;

        float u0 = bounds->left / atlasWidth;
        float v0 = bounds->top / atlasHeight;
        float u1 = bounds->right / atlasWidth;
        float v1 = bounds->bottom / atlasHeight;

        if (!avdDrawListAddRectFilledUv(
                drawList,
                (AVD_Vector2){ax, ay}, (AVD_Vector2){bx, by},
                (AVD_Vector2){u0, v0}, (AVD_Vector2){u1, v1},
                color)) {
            AVD_LOG_WARN("Failed to add text to draw list");
        }

        cX += glyph->advanceX * fontSize;
    }

    avdDrawListPopTexture(drawList);

    return true;
}

// ----------------

AVD_Bool avdDrawListRendererCreate(AVD_DrawListRenderer *renderer, AVD_VulkanFramebuffer *framebuffer, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(framebuffer != NULL);

    memset(renderer, 0, sizeof(AVD_DrawListRenderer));

    AVD_CHECK(PRIV_avdDrawListRendererSetupDescriptors(renderer, vulkan));
    AVD_CHECK(avdAllocateDescriptorSet(
        vulkan->device,
        vulkan->descriptorPool,
        renderer->bufferDescriptorSetLayout,
        &renderer->bufferDescriptorSet));
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_DESCRIPTOR_SET,
        renderer->bufferDescriptorSet,
        "[DescriptorSet][Common]:DrawList/Buffer");

    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &renderer->pipelineLayout,
        &renderer->pipeline,
        vulkan->device,
        (VkDescriptorSetLayout[]){
            renderer->bufferDescriptorSetLayout,
            renderer->textureDescriptorSetLayout},
        2,
        sizeof(AVD_DrawListPushConstants),
        framebuffer->renderPass,
        (AVD_UInt32)framebuffer->colorAttachments.count,
        "DrawListVert",
        "DrawListFrag",
        NULL,
        NULL));
    return true;
}

void avdDrawListRendererDestroy(AVD_DrawListRenderer *renderer, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(vulkan != NULL);

    avdVulkanBufferDestroy(vulkan, &renderer->vertexBuffer);
    vkDestroyPipeline(vulkan->device, renderer->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, renderer->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, renderer->bufferDescriptorSetLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, renderer->textureDescriptorSetLayout, NULL);
    memset(renderer, 0, sizeof(AVD_DrawListRenderer));
}

AVD_Bool avdDrawListRendererRender(
    AVD_DrawListRenderer *renderer,
    AVD_Vulkan *vulkan,
    VkCommandBuffer commandBuffer,
    AVD_DrawListPacked *drawListData)
{
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(drawListData != NULL);

    AVD_CHECK(PRIV_avdDrawListRendererResizeBuffersIfNeeded(renderer, drawListData, vulkan));

    // upload vertex data
    avdVulkanBufferUpload(
        vulkan,
        &renderer->vertexBuffer,
        drawListData->vertices,
        drawListData->vertexCount * sizeof(AVD_ModelVertexPacked));

    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][DrawList]:Render");
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderer->pipelineLayout,
        0,
        1,
        &renderer->bufferDescriptorSet,
        0,
        NULL);

    for (AVD_UInt32 i = 0; i < drawListData->commandCount; ++i) {
        AVD_DrawListCommand *cmd = &drawListData->commands[i];

        AVD_CHECK_MSG(cmd != NULL, "DrawList command is NULL");

        AVD_DEBUG_VK_CMD_INSERT_LABEL(commandBuffer, NULL, "[Cmd][DrawList]:RenderCommand-%d", i);

        AVD_DrawListPushConstants pushConstants = {
            .vertexOffset      = cmd->vertexOffset,
            .framebufferWidth  = drawListData->width,
            .framebufferHeight = drawListData->height,
            .fontPxRange       = cmd->fontPxRange,
        };
        vkCmdPushConstants(
            commandBuffer,
            renderer->pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(AVD_DrawListPushConstants),
            &pushConstants);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->pipelineLayout,
            1,
            1,
            (VkDescriptorSet[]){(VkDescriptorSet)(cmd->textureHandle)},
            0,
            NULL);
        vkCmdDraw(commandBuffer, cmd->vertexCount, 1, 0, 0);
    }

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);
    return true;
}
