#include "common/avd_drawlist.h"
#include "core/avd_base.h"
#include "core/avd_list.h"
#include "core/avd_types.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/avd_vulkan_framebuffer.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"

typedef struct AVD_DrawListPushConstants {
    AVD_UInt32 vertexOffset;
    AVD_UInt32 indexOffset;
    AVD_UInt32 indexCount;
} AVD_DrawListPushConstants;

bool avdDrawListCreate(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    memset(drawList, 0, sizeof(AVD_DrawList));

    avdListCreate(&drawList->commands, sizeof(AVD_DrawListCommand));
    avdListCreate(&drawList->vertexData, sizeof(AVD_ModelVertexPacked));
    avdListCreate(&drawList->indexData, sizeof(AVD_UInt32));

    return true;
}

void avdDrawListDestroy(AVD_DrawList *drawList)
{
    AVD_ASSERT(drawList != NULL);

    avdListDestroy(&drawList->commands);
    avdListDestroy(&drawList->vertexData);
    avdListDestroy(&drawList->indexData);

    memset(drawList, 0, sizeof(AVD_DrawList));
}

AVD_Bool avdDrawListPack(AVD_DrawList *drawList, AVD_DrawListPacked *packedData)
{
    AVD_ASSERT(drawList != NULL);
    AVD_ASSERT(packedData != NULL);

    memset(packedData, 0, sizeof(AVD_DrawListPacked));

    // NOTE: for now we dont necessarily need to pack as the data
    // as already being added in a packed format. However, later
    // we might implement layer merging optimizaitons where packing
    // will actually involved modifying the data.

    packedData->vertices     = drawList->vertexData.items;
    packedData->vertexCount  = drawList->vertexData.count;
    packedData->indices      = drawList->indexData.items;
    packedData->indexCount   = drawList->indexData.count;
    packedData->commands     = drawList->commands.items;
    packedData->commandCount = drawList->commands.count;
    return true;
}

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
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 2,
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
    AVD_Size requiredIndexBufferSize  = AVD_MAX(drawListData->indexCount * sizeof(AVD_UInt32), 4);

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
    }

    if (renderer->indexBuffer.size < requiredIndexBufferSize || !renderer->indexBuffer.initialized) {
        if (renderer->indexBuffer.initialized) {
            avdVulkanBufferDestroy(vulkan, &renderer->indexBuffer);
        }
        AVD_CHECK(
            avdVulkanBufferCreate(
                vulkan,
                &renderer->indexBuffer,
                requiredIndexBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                // NOTE: since we will be updating these buffers from the CPU every frame,
                // having them device local wont really help much, and it would require
                // an extra staging buffer for the uploads
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                "DrawList/IndexBuffer"));
    }

    return true;
}

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
    avdVulkanBufferDestroy(vulkan, &renderer->indexBuffer);
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

    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][DrawList]:Render");
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipelineLayout, 0, 1, &renderer->bufferDescriptorSet, 0, NULL);

    for (AVD_UInt32 i = 0; i < drawListData->commandCount; ++i) {
        AVD_DrawListCommand *cmd = &drawListData->commands[i];

        AVD_CHECK_MSG(cmd != NULL, "DrawList command is NULL");

        AVD_DEBUG_VK_CMD_INSERT_LABEL(commandBuffer, NULL, "[Cmd][DrawList]:RenderCommand-%d", i);

        AVD_DrawListPushConstants pushConstants = {
            .vertexOffset = cmd->vertexOffset,
            .indexOffset  = cmd->indexOffset,
            .indexCount   = cmd->indexCount};
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
            &renderer->bufferDescriptorSet,
            0,
            NULL);
        vkCmdDraw(commandBuffer, cmd->indexCount, 1, 0, 0);
    }

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);
    return true;
}
