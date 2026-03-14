#include "common/avd_drawlist.h"
#include "core/avd_base.h"
#include "core/avd_list.h"
#include "core/avd_types.h"
#include "font/avd_font_renderer.h"
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
    AVD_UInt32 pad0;
} AVD_DrawListPushConstants;

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
    avdListPushBack(&drawList->commands, &drawList->currentCommand);
    drawList->currentCommand.vertexOffset = (AVD_UInt32)drawList->vertexData.count;
    drawList->currentCommand.vertexCount  = 0;

    return true;
}

static AVD_Bool PRIV_avdDrawListAddTriangle(
    AVD_DrawList *drawList,
    AVD_ModelVertex vertices[3],
    void *textureHandle)
{
    AVD_ASSERT(drawList != NULL);

    AVD_CHECK_MSG(drawList->recording, "Must call avdDrawListBegin before adding draw commands");

    if (textureHandle == NULL) {
        textureHandle = drawList->currentCommand.textureHandle;
    }

    if (drawList->currentCommand.textureHandle != textureHandle) {
        AVD_CHECK(PRIV_avdDrawListFinalizeCurrentCommand(drawList));
    }

    AVD_ModelVertexPacked packedVertex = {0};
    for (int i = 0; i < 3; ++i) {
        avdModelVertexPack(&vertices[i], &packedVertex);
        avdListPushBack(&drawList->vertexData, &packedVertex);
    }

    drawList->currentCommand.textureHandle = textureHandle;

    return true;
}

static void *PRIV_avdDrawListGetCurrentTexture(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    if (drawList->textureStackTop >= 0) {
        return drawList->textureStack[drawList->textureStackTop];
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
    drawList->textureStack[drawList->textureStackTop] = textureHandle;

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

    drawList->recording                   = true;
    drawList->framebufferWidth            = width;
    drawList->framebufferHeight           = height;
    drawList->currentCommand.vertexOffset = 0;
    drawList->currentCommand.vertexCount  = 0;

    AVD_Font *defaultFont = NULL;
    AVD_CHECK(avdFontManagerGetFont(drawList->fontManager, "Default", &defaultFont));
    drawList->currentCommand.textureHandle = (void *)defaultFont->fontDescriptorSet;

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

AVD_Bool avdDrawListAddTriangle(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos, AVD_Vector2 v1UV,
    AVD_Vector2 v2Pos, AVD_Vector2 v2UV,
    AVD_Vector2 v3Pos, AVD_Vector2 v3UV,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);
    AVD_CHECK_MSG(drawList->recording, "Must call avdDrawListBegin before adding draw commands");

    void *currentTexture = PRIV_avdDrawListGetCurrentTexture(drawList);

    AVD_ModelVertex vertices[3] = {
        {
            .position = {
                v1Pos.x,
                v1Pos.y,
                currentTexture != NULL ? 1.0f : 0.0f,
            },
            .normal   = {color.x, color.y, color.z},
            .texCoord = {v1UV.x, v1UV.y},
        },
        {
            .position = {
                v2Pos.x,
                v2Pos.y,
                currentTexture != NULL ? 1.0f : 0.0f,
            },
            .normal   = {color.x, color.y, color.z},
            .texCoord = {v2UV.x, v2UV.y},
        },
        {
            .position = {
                v3Pos.x, v3Pos.y,
                currentTexture != NULL ? 1.0f : 0.0f, // if no texture, it will just use the color
            },
            .normal   = {color.x, color.y, color.z},
            .texCoord = {v3UV.x, v3UV.y},
        },
    };

    AVD_CHECK(
        PRIV_avdDrawListAddTriangle(
            drawList,
            vertices,
            currentTexture));

    return true;
}

AVD_Bool avdDrawListAddQuadUv(
    AVD_DrawList *drawList,
    AVD_Vector2 v1Pos, AVD_Vector2 v1UV,
    AVD_Vector2 v2Pos, AVD_Vector2 v2UV,
    AVD_Vector2 v3Pos, AVD_Vector2 v3UV,
    AVD_Vector2 v4Pos, AVD_Vector2 v4UV,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);

    AVD_CHECK(avdDrawListAddTriangle(drawList, v1Pos, v1UV, v2Pos, v2UV, v3Pos, v3UV, color));
    AVD_CHECK(avdDrawListAddTriangle(drawList, v1Pos, v1UV, v3Pos, v3UV, v4Pos, v4UV, color));
    return true;
}

bool avdDrawListAddQuad(
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
    AVD_CHECK(avdDrawListAddQuadUv(
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

AVD_Bool avdDrawListAddRectUv(
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

    AVD_CHECK(avdDrawListAddQuadUv(drawList, v1Pos, v1UV, v2Pos, v2UV, v3Pos, v3UV, v4Pos, v4UV, color));
    return true;
}

bool avdDrawListAddRect(
    AVD_DrawList *drawList,
    AVD_Vector2 minPos, AVD_Vector2 maxPos,
    AVD_Vector3 color)
{
    AVD_ASSERT(drawList != NULL);

    AVD_Vector2 uvMin = {0.0f, 0.0f};
    AVD_Vector2 uvMax = {1.0f, 1.0f};
    AVD_CHECK(avdDrawListAddRectUv(
        drawList,
        minPos,
        maxPos,
        uvMin,
        uvMax,
        color));

    return true;
}

AVD_Bool avdDrawListAddCircleUv(
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

    for (AVD_UInt32 i = 1; i <= segmentCount; i++) {
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

        AVD_CHECK(avdDrawListAddTriangle(drawList, center, uvCenter, prevPos, prevUV, nextPos, nextUV, color));

        prevPos = nextPos;
        prevUV  = nextUV;
    }
    return true;
}

bool avdDrawListAddCircle(
    AVD_DrawList *drawList,
    AVD_Vector2 center, float radius,
    AVD_Vector3 color,
    int segmentCount)
{
    AVD_ASSERT(drawList != NULL);

    AVD_Vector2 uvCenter = {0.5f, 0.5f};
    float uvRadius       = 0.5f;
    return avdDrawListAddCircleUv(
        drawList,
        center,
        radius,
        uvCenter,
        uvRadius,
        color,
        segmentCount);
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
            .pad0              = 0};
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
