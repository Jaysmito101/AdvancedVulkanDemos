#include "ui/avd_ui.h"
#include "avd_application.h"

typedef struct AVD_UiPushConstants
{
    int type;
    float width;
    float height;
    float radius;

    float offsetX;
    float offsetY;
    float frameWidth;
    float frameHeight;

    float imageWidth;
    float imageHeight;
    float pad0;
    float pad1;

    float uiBoxMinX;
    float uiBoxMinY;
    float uiBoxMaxX;
    float uiBoxMaxY;

    float colorR;
    float colorG;
    float colorB;
    float colorA;
} AVD_UiPushConstants;

bool avdUiInit(AVD_Ui *ui, struct AVD_AppState *appState)
{
    AVD_ASSERT(ui != NULL);
    AVD_ASSERT(appState != NULL);

    AVD_Vulkan *vulkan = &appState->vulkan;

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &ui->descriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));
    AVD_CHECK(avdPipelineUtilsCreateGraphicsPipelineLayout(
        &ui->pipelineLayout,
        vulkan->device,
        &ui->descriptorSetLayout, 1,
        sizeof(AVD_UiPushConstants)));
    AVD_CHECK(avdPipelineUtilsCreateGenericGraphicsPipeline(
        &ui->pipeline,
        ui->pipelineLayout,
        vulkan->device,
        appState->renderer.sceneFramebuffer.renderPass,
        "FullScreenQuadVert",
        "UiFrag"));

    return true;
}

void avdUiDestroy(AVD_Ui *ui, struct AVD_AppState *appState)
{
    AVD_ASSERT(ui != NULL);
    AVD_ASSERT(appState != NULL);

    AVD_Vulkan *vulkan = &appState->vulkan;

    vkDestroyPipeline(vulkan->device, ui->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, ui->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, ui->descriptorSetLayout, NULL);
}

void avdUiBegin(VkCommandBuffer commandBuffer, AVD_Ui *ui, AVD_AppState *appState, float width, float height, float offsetX, float offsetY, uint32_t frameWidth, uint32_t frameHeight)
{
    AVD_ASSERT(ui != NULL);
    AVD_ASSERT(appState != NULL);

    ui->width = width;
    ui->height = height;
    ui->offsetX = offsetX;
    ui->offsetY = offsetY;
    ui->frameWidth = (float)frameWidth;
    ui->frameHeight = (float)frameHeight;

    float minX = AVD_MAX(offsetX, 0.0f);
    float minY = AVD_MAX(offsetY, 0.0f);
    float maxX = AVD_MIN(offsetX + width, (float)frameWidth);
    float maxY = AVD_MIN(offsetY + height, (float)frameHeight);

    VkRect2D scissor = {
        .offset = {(int32_t)minX, (int32_t)minY},
        .extent = {(uint32_t)(maxX - minX), (uint32_t)(maxY - minY)},
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void avdUiEnd(VkCommandBuffer commandBuffer, AVD_Ui *ui, AVD_AppState *appState)
{
    AVD_ASSERT(ui != NULL);
    AVD_ASSERT(appState != NULL);

    // A No op for now?
}

void avdUiDrawRect(
    VkCommandBuffer commandBuffer,
    AVD_Ui *ui,
    struct AVD_AppState *appState,
    float x, float y,
    float width, float height,
    float r, float g, float b, float a,
    VkDescriptorSet descriptorSet, uint32_t imageWidth, uint32_t imageHeight)
{
    AVD_ASSERT(ui != NULL);
    AVD_ASSERT(appState != NULL);

    // TODO: THIS IS A HACK!! Fix it with a proper descriptor set for the fallback image
    VkDescriptorSet fallbackImage = appState->fontRenderer.fonts[0].fontDescriptorSet;
    float fallbackImageWidth = (float)imageWidth;
    float fallbackImageHeight = (float)imageHeight;

    if (descriptorSet != NULL)
    {
        fallbackImage = descriptorSet;
    }

    AVD_UiPushConstants pushConstants = {
        .type = AVD_UI_ELEMENT_TYPE_RECT,
        .width = ui->width,
        .height = ui->height,
        .radius = 0.0f,
        .offsetX = ui->offsetX,
        .offsetY = ui->offsetY,
        .frameWidth = ui->frameWidth,
        .frameHeight = ui->frameHeight,
        .uiBoxMinX = (x + ui->offsetX) / ui->frameWidth,
        .uiBoxMinY = (y + ui->offsetY) / ui->frameHeight,
        .uiBoxMaxX = (x + width + ui->offsetX) / ui->frameWidth,
        .uiBoxMaxY = (y + height + ui->offsetY) / ui->frameHeight,
        .imageWidth = fallbackImageWidth,
        .imageHeight = fallbackImageHeight,
        .pad0 = 0.0f,
        .pad1 = 0.0f,
        .colorR = r,
        .colorG = g,
        .colorB = b,
        .colorA = a,
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ui->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ui->pipelineLayout, 0, 1, &fallbackImage, 0, NULL);
    vkCmdPushConstants(commandBuffer, ui->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}