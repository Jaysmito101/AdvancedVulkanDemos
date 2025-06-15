#include "vulkan/avd_vulkan_pipeline_utils.h"
#include "shader/avd_shader.h"
#include "vulkan/avd_vulkan_renderer.h"

bool avdPipelineUtilsShaderStage(VkPipelineShaderStageCreateInfo *shaderStageInfo, VkShaderModule shaderModule, VkShaderStageFlagBits stageFlags)
{
    AVD_ASSERT(shaderStageInfo != NULL);
    AVD_ASSERT(shaderModule != VK_NULL_HANDLE);

    shaderStageInfo->sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo->stage  = stageFlags;
    shaderStageInfo->module = shaderModule;
    shaderStageInfo->pName  = "main";

    return true;
}

bool avdPipelineUtilsDynamicState(VkPipelineDynamicStateCreateInfo *dynamicStateInfo)
{
    AVD_ASSERT(dynamicStateInfo != NULL);

    static VkDynamicState dynamicStates[2] = {0};
    dynamicStates[0]                       = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStates[1]                       = VK_DYNAMIC_STATE_SCISSOR;

    dynamicStateInfo->sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo->pNext             = NULL;
    dynamicStateInfo->flags             = 0;
    dynamicStateInfo->pDynamicStates    = dynamicStates;
    dynamicStateInfo->dynamicStateCount = AVD_ARRAY_COUNT(dynamicStates);

    return true;
}

bool avdPipelineUtilsInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo *inputAssemblyInfo)
{
    inputAssemblyInfo->sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo->topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo->primitiveRestartEnable = VK_FALSE;

    return true;
}

bool avdPipelineUtilsViewportScissor(VkViewport *viewport, VkRect2D *scissor)
{
    AVD_ASSERT(viewport != NULL);
    AVD_ASSERT(scissor != NULL);

    viewport->x        = 0.0f;
    viewport->y        = 0.0f;
    viewport->width    = 1024.0f;
    viewport->height   = 1024.0f;
    viewport->minDepth = 0.0f;
    viewport->maxDepth = 1.0f;

    scissor->offset.x      = 0;
    scissor->offset.y      = 0;
    scissor->extent.width  = 1024;
    scissor->extent.height = 1024;

    return true;
}

bool avdPipelineUtilsViewportState(VkPipelineViewportStateCreateInfo *viewportStateInfo, VkViewport *viewport, VkRect2D *scissor)
{
    AVD_ASSERT(viewportStateInfo != NULL);
    AVD_ASSERT(viewport != NULL);
    AVD_ASSERT(scissor != NULL);

    viewportStateInfo->sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo->viewportCount = 1;
    viewportStateInfo->pViewports    = viewport;
    viewportStateInfo->scissorCount  = 1;
    viewportStateInfo->pScissors     = scissor;

    return true;
}

bool avdPipelineUtilsRasterizationState(VkPipelineRasterizationStateCreateInfo *rasterizerInfo)
{
    AVD_ASSERT(rasterizerInfo != NULL);

    rasterizerInfo->sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo->depthClampEnable        = VK_FALSE;
    rasterizerInfo->rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo->polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizerInfo->cullMode                = VK_CULL_MODE_NONE;
    rasterizerInfo->frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo->depthBiasEnable         = VK_FALSE;
    rasterizerInfo->lineWidth               = 1.0f;

    return true;
}

bool avdPipelineUtilsMultisampleState(VkPipelineMultisampleStateCreateInfo *multisampleInfo)
{
    AVD_ASSERT(multisampleInfo != NULL);

    multisampleInfo->sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo->rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo->sampleShadingEnable   = VK_FALSE;
    multisampleInfo->minSampleShading      = 1.0f;
    multisampleInfo->pSampleMask           = NULL;
    multisampleInfo->alphaToCoverageEnable = VK_FALSE;
    multisampleInfo->alphaToOneEnable      = VK_FALSE;

    return true;
}

bool avdPipelineUtilsDepthStencilState(VkPipelineDepthStencilStateCreateInfo *depthStencilInfo, bool enableDepthTest)
{
    AVD_ASSERT(depthStencilInfo != NULL);

    depthStencilInfo->sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo->depthTestEnable       = enableDepthTest ? VK_TRUE : VK_FALSE;
    depthStencilInfo->depthWriteEnable      = enableDepthTest ? VK_TRUE : VK_FALSE;
    depthStencilInfo->depthCompareOp        = VK_COMPARE_OP_ALWAYS;
    depthStencilInfo->depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo->stencilTestEnable     = VK_FALSE;

    return true;
}

bool avdPipelineUtilsBlendAttachment(VkPipelineColorBlendAttachmentState *blendAttachment, bool enableBlend)
{
    AVD_ASSERT(blendAttachment != NULL);

    blendAttachment->blendEnable         = enableBlend ? VK_TRUE : VK_FALSE;
    blendAttachment->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment->colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachment->srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment->alphaBlendOp        = VK_BLEND_OP_ADD;
    blendAttachment->colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    return true;
}

bool avdPipelineUtilsColorBlendState(VkPipelineColorBlendStateCreateInfo *colorBlendStateInfo, VkPipelineColorBlendAttachmentState *blendAttachments, size_t attachmentCount)
{
    AVD_ASSERT(colorBlendStateInfo != NULL);
    AVD_ASSERT(blendAttachments != NULL);
    AVD_ASSERT(attachmentCount > 0);

    colorBlendStateInfo->sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo->logicOpEnable   = VK_FALSE;
    colorBlendStateInfo->logicOp         = VK_LOGIC_OP_COPY;
    colorBlendStateInfo->attachmentCount = (uint32_t)attachmentCount;
    colorBlendStateInfo->pAttachments    = blendAttachments;

    return true;
}

bool avdPipelineUtilsCreateGraphicsPipelineLayout(
    VkPipelineLayout *pipelineLayout,
    VkDevice device,
    VkDescriptorSetLayout *descriptorSetLayouts,
    size_t descriptorSetLayoutCount,
    uint32_t pushConstantSize)
{
    AVD_ASSERT(pipelineLayout != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset              = 0;
    pushConstantRanges[0].size                = pushConstantSize;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount             = (uint32_t)descriptorSetLayoutCount;
    pipelineLayoutInfo.pSetLayouts                = descriptorSetLayouts;
    pipelineLayoutInfo.pPushConstantRanges        = pushConstantRanges;
    pipelineLayoutInfo.pushConstantRangeCount     = AVD_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, pipelineLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create pipeline layout for presentation");

    return true;
}

bool avdPipelineUtilsCreateGenericGraphicsPipeline(VkPipeline *pipeline, VkPipelineLayout layout, VkDevice device, VkRenderPass renderPass, const char *vertShaderAsset, const char *fragShaderAsset)
{
    AVD_ASSERT(pipeline != NULL);
    AVD_ASSERT(layout != VK_NULL_HANDLE);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);
    AVD_ASSERT(vertShaderAsset != NULL);
    AVD_ASSERT(fragShaderAsset != NULL);

    VkShaderModule vertexShaderModule = avdShaderModuleCreateFromAsset(device, vertShaderAsset);
    AVD_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");

    VkShaderModule fragmentShaderModule = avdShaderModuleCreateFromAsset(device, fragShaderAsset);
    AVD_CHECK_VK_HANDLE(fragmentShaderModule, "Failed to create fragment shader module\n");

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    AVD_CHECK(avdPipelineUtilsShaderStage(&shaderStages[0], vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT));
    AVD_CHECK(avdPipelineUtilsShaderStage(&shaderStages[1], fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT));

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {0};
    AVD_CHECK(avdPipelineUtilsDynamicState(&dynamicStateInfo));

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {0};
    AVD_CHECK(avdPipelineUtilsInputAssemblyState(&inputAssemblyInfo));

    VkViewport viewport = {0};
    VkRect2D scissor    = {0};
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
    vertexInputInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount        = 0;
    vertexInputInfo.vertexAttributeDescriptionCount      = 0;

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount                   = AVD_ARRAY_COUNT(shaderStages);
    pipelineInfo.pStages                      = shaderStages;
    pipelineInfo.layout                       = layout;
    pipelineInfo.renderPass                   = renderPass;
    pipelineInfo.subpass                      = 0;
    pipelineInfo.pVertexInputState            = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState          = &inputAssemblyInfo;
    pipelineInfo.pViewportState               = &viewportStateInfo;
    pipelineInfo.pRasterizationState          = &rasterizerInfo;
    pipelineInfo.pMultisampleState            = &multisampleInfo;
    pipelineInfo.pDepthStencilState           = &depthStencilInfo;
    pipelineInfo.pColorBlendState             = &colorBlendStateInfo;
    pipelineInfo.pDynamicState                = &dynamicStateInfo;
    pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex            = -1;
    pipelineInfo.pDepthStencilState           = &depthStencilInfo;
    pipelineInfo.pViewportState               = &viewportStateInfo;
    pipelineInfo.pMultisampleState            = &multisampleInfo;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics pipeline for Ui");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool avdWriteImageDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo *imageInfo)
{
    AVD_ASSERT(writeDescriptorSet != NULL);
    AVD_ASSERT(descriptorSet != VK_NULL_HANDLE);
    AVD_ASSERT(imageInfo != NULL);

    writeDescriptorSet->sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet->pNext            = NULL;
    writeDescriptorSet->dstSet           = descriptorSet;
    writeDescriptorSet->dstBinding       = binding;
    writeDescriptorSet->dstArrayElement  = 0;
    writeDescriptorSet->descriptorCount  = 1;
    writeDescriptorSet->descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet->pImageInfo       = imageInfo;
    writeDescriptorSet->pBufferInfo      = NULL;
    writeDescriptorSet->pTexelBufferView = NULL;

    return true;
}

// TODO: Ideally this shouldn't return the ownership of the descriptor set layout
// but rather just cache it in AVD_Vulkan or something similar and sort of
// have a cache layer for descriptor set layouts.
bool avdCreateDescriptorSetLayout(
    VkDescriptorSetLayout *descriptorSetLayout,
    VkDevice device,
    VkDescriptorType *descriptorTypes,
    size_t descriptorTypesCount,
    VkShaderStageFlags stageFlags)
{
    AVD_ASSERT(descriptorSetLayout != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(descriptorTypes != NULL);
    AVD_ASSERT(descriptorTypesCount > 0);
    AVD_ASSERT(descriptorTypesCount <= AVD_MAX_DESCRIPTOR_SET_BINDINGS);

    static VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[AVD_MAX_DESCRIPTOR_SET_BINDINGS] = {0};
    for (size_t i = 0; i < descriptorTypesCount; ++i) {
        descriptorSetLayoutBindings[i].binding         = (uint32_t)i;
        descriptorSetLayoutBindings[i].descriptorType  = descriptorTypes[i];
        descriptorSetLayoutBindings[i].descriptorCount = 1;
        descriptorSetLayoutBindings[i].stageFlags      = stageFlags;
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {0};
    descriptorSetLayoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount                    = (uint32_t)descriptorTypesCount;
    descriptorSetLayoutInfo.pBindings                       = descriptorSetLayoutBindings;

    VkResult result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, NULL, descriptorSetLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create descriptor set layout");

    return true;
}

bool avdWriteBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo)
{
    AVD_ASSERT(writeDescriptorSet != NULL);
    AVD_ASSERT(descriptorSet != VK_NULL_HANDLE);
    AVD_ASSERT(bufferInfo != NULL);

    writeDescriptorSet->sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet->pNext            = NULL;
    writeDescriptorSet->dstSet           = descriptorSet;
    writeDescriptorSet->dstBinding       = binding;
    writeDescriptorSet->dstArrayElement  = 0;
    writeDescriptorSet->descriptorCount  = 1;
    writeDescriptorSet->descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet->pImageInfo       = NULL;
    writeDescriptorSet->pBufferInfo      = bufferInfo;
    writeDescriptorSet->pTexelBufferView = NULL;

    return true;
}

bool avdBeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, const VkImageView *attachments, size_t attachmentCount, uint32_t framebufferWidth, uint32_t framebufferHeight, VkClearValue *customClearValues, size_t customClearValueCount)
{
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);
    AVD_ASSERT(framebuffer != VK_NULL_HANDLE);
    AVD_ASSERT(framebufferWidth > 0);
    AVD_ASSERT(framebufferHeight > 0);

    static VkClearValue defaultClearValues[2] = {
        {.color = {.float32 = {10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f, 1.0f}}},
        {.depthStencil = {.depth = 1.0f, .stencil = 0}}};

    VkClearValue *clearValues = NULL;
    size_t clearValueCount    = 0;

    if (customClearValues != NULL && customClearValueCount > 0) {
        clearValues     = customClearValues;
        clearValueCount = customClearValueCount;
    } else {
        clearValues     = defaultClearValues;
        clearValueCount = AVD_ARRAY_COUNT(defaultClearValues);
    }

    VkRenderPassAttachmentBeginInfo attachmentBeginInfo = {0};
    attachmentBeginInfo.sType                           = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
    attachmentBeginInfo.attachmentCount                 = (uint32_t)attachmentCount;
    attachmentBeginInfo.pAttachments                    = attachments;

    VkRenderPassBeginInfo renderPassInfo    = {0};
    renderPassInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass               = renderPass;
    renderPassInfo.framebuffer              = framebuffer;
    renderPassInfo.renderArea.offset.x      = 0;
    renderPassInfo.renderArea.offset.y      = 0;
    renderPassInfo.renderArea.extent.width  = framebufferWidth;
    renderPassInfo.renderArea.extent.height = framebufferHeight;
    renderPassInfo.clearValueCount          = (uint32_t)clearValueCount;
    renderPassInfo.pClearValues             = clearValues;
    renderPassInfo.pNext                    = &attachmentBeginInfo;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {0};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = (float)framebufferWidth;
    viewport.height     = (float)framebufferHeight;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor      = {0};
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = framebufferWidth;
    scissor.extent.height = framebufferHeight;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    return true;
}

bool avdEndRenderPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
    return true;
}

bool avdBeginSceneRenderPass(VkCommandBuffer commandBuffer, AVD_VulkanRenderer *renderer)
{
    AVD_ASSERT(renderer != NULL);
    static VkImageView attachments[16] = {0};
    static size_t attachmentCount      = 0;

    AVD_CHECK(avdVulkanFramebufferGetAttachmentViews(
        &renderer->sceneFramebuffer,
        attachments,
        &attachmentCount));

    avdBeginRenderPass(
        commandBuffer,
        renderer->sceneFramebuffer.renderPass,
        renderer->sceneFramebuffer.framebuffer,
        attachments,
        attachmentCount,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height,
        NULL,
        0);
    return true;
}

bool avdEndSceneRenderPass(VkCommandBuffer commandBuffer)
{
    avdEndRenderPass(commandBuffer);
    return true;
}