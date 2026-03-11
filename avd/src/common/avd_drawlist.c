#include "common/avd_drawlist.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "vulkan/avd_vulkan_framebuffer.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"

typedef struct AVD_DrawListPushConstants {
    AVD_UInt32 temp;
} AVD_DrawListPushConstants;

AVD_Bool avdDrawListRendererCreate(AVD_DrawListRenderer *renderer, AVD_VulkanFramebuffer *framebuffer, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(framebuffer != NULL);

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &renderer->descriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));

    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &renderer->pipelineLayout,
        &renderer->pipeline,
        vulkan->device,
        (VkDescriptorSetLayout[]){renderer->descriptorSetLayout},
        1,
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

    vkDestroyPipeline(vulkan->device, renderer->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, renderer->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, renderer->descriptorSetLayout, NULL);
}

AVD_Bool avdDrawListRendererRender(AVD_DrawListRenderer *renderer, VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet)
{
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][DrawList]:Render");

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);
    return true;
}
