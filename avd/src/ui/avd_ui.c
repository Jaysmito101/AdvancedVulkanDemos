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

    float uiBoxMinX;
    float uiBoxMinY;
    float uiBoxMaxX;
    float uiBoxMaxY;

    float colorR;
    float colorG;
    float colorB;
    float colorA;
} AVD_UiPushConstants;

static bool __avdUiSetupDescriptors(AVD_Ui *ui, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(ui != NULL);

    VkDescriptorSetLayoutBinding sceneFramebufferBinding = {0};
    sceneFramebufferBinding.binding = 0;
    sceneFramebufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sceneFramebufferBinding.descriptorCount = 1;
    sceneFramebufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo sceneFramebufferLayoutInfo = {0};
    sceneFramebufferLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sceneFramebufferLayoutInfo.bindingCount = 1;
    sceneFramebufferLayoutInfo.pBindings = &sceneFramebufferBinding;

    VkResult sceneLayoutResult = vkCreateDescriptorSetLayout(vulkan->device, &sceneFramebufferLayoutInfo, NULL, &ui->descriptorSetLayout);
    AVD_CHECK_VK_RESULT(sceneLayoutResult, "Failed to create scene framebuffer descriptor set layout");
    return true;
}

static bool __avdUiCreatePipelineLayout(AVD_Ui *ui, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(ui != NULL);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(AVD_UiPushConstants); // Use sizeof the struct

    // Combine layouts for the pipeline layout
    VkDescriptorSetLayout setLayouts[] = {
        ui->descriptorSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = AVD_ARRAY_COUNT(setLayouts);
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    pipelineLayoutInfo.pushConstantRangeCount = AVD_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(vulkan->device, &pipelineLayoutInfo, NULL, &ui->pipelineLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create pipeline layout for Ui");

    return true;
}

static bool __avdUiCreatePipeline(AVD_Ui *ui, VkDevice device, VkRenderPass renderPass)
{
    AVD_ASSERT(ui != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = avdShaderModuleCreateFromAsset(device, "FullScreenQuadVert");
    AVD_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");

    VkShaderModule fragmentShaderModule = avdShaderModuleCreateFromAsset(device, "UiFrag");
    AVD_CHECK_VK_HANDLE(fragmentShaderModule, "Failed to create fragment shader module\n");

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    AVD_CHECK(avdPipelineUtilsShaderStage(&shaderStages[0], vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT));
    AVD_CHECK(avdPipelineUtilsShaderStage(&shaderStages[1], fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT));

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {0};
    AVD_CHECK(avdPipelineUtilsDynamicState(&dynamicStateInfo));

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {0};
    AVD_CHECK(avdPipelineUtilsInputAssemblyState(&inputAssemblyInfo));

    VkViewport viewport = {0};
    VkRect2D scissor = {0};
    AVD_CHECK(avdPipelineUtilsViewportScissor(&viewport, &scissor));

    VkPipelineViewportStateCreateInfo viewportStateInfo = {0};
    AVD_CHECK(avdPipelineUtilsViewportState(&viewportStateInfo, &viewport, &scissor));

    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {0};
    AVD_CHECK(avdPipelineUtilsRasterizationState(&rasterizerInfo));

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {0};
    AVD_CHECK(avdPipelineUtilsMultisampleState(&multisampleInfo));

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {0};
    AVD_CHECK(avdPipelineUtilsDepthStencilState(&depthStencilInfo, false));

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    AVD_CHECK(avdPipelineUtilsBlendAttachment(&colorBlendAttachment, true));

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {0};
    AVD_CHECK(avdPipelineUtilsColorBlendState(&colorBlendStateInfo, &colorBlendAttachment, 1));

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = AVD_ARRAY_COUNT(shaderStages);
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.layout = ui->pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendStateInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ui->pipeline);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics pipeline for Ui");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool avdUiInit(AVD_Ui *ui, struct AVD_AppState *appState)
{
    AVD_ASSERT(ui != NULL);
    AVD_ASSERT(appState != NULL);

    AVD_Vulkan *vulkan = &appState->vulkan;

    AVD_CHECK(__avdUiSetupDescriptors(ui, vulkan));
    AVD_CHECK(__avdUiCreatePipelineLayout(ui, vulkan));
    AVD_CHECK(__avdUiCreatePipeline(ui, vulkan->device, appState->renderer.sceneFramebuffer.renderPass));

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

void avdUiDrawRect(VkCommandBuffer commandBuffer, AVD_Ui *ui, AVD_AppState *appState, float x, float y, float width, float height, float r, float g, float b, float a)
{
    AVD_ASSERT(ui != NULL);
    AVD_ASSERT(appState != NULL);

    // TODO: THIS IS A HACK!! Fix it with a proper descriptor set for the fallback image
    VkDescriptorSet fallbackImage = appState->fontRenderer.fonts[0].fontDescriptorSet;

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
        .colorR = r,
        .colorG = g,
        .colorB = b,
        .colorA = a,
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ui->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ui->pipelineLayout, 0, 1, &fallbackImage, 0, NULL);
    vkCmdPushConstants(commandBuffer, ui->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

}