#include "ps_pipeline_utils.h"

bool psPipelineUtilsShaderStage(VkPipelineShaderStageCreateInfo *shaderStageInfo, VkShaderModule shaderModule, VkShaderStageFlagBits stageFlags) {
    PS_ASSERT(shaderStageInfo != NULL);
    PS_ASSERT(shaderModule != VK_NULL_HANDLE);

    shaderStageInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo->stage = stageFlags;
    shaderStageInfo->module = shaderModule;
    shaderStageInfo->pName = "main";

    return true;
}

bool psPipelineUtilsDynamicState(VkPipelineDynamicStateCreateInfo *dynamicStateInfo)
{
    PS_ASSERT(dynamicStateInfo != NULL);

    static VkDynamicState dynamicStates[2] = {0};
    dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;
    
    dynamicStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo->pNext = NULL;
    dynamicStateInfo->flags = 0;
    dynamicStateInfo->pDynamicStates = dynamicStates;
    dynamicStateInfo->dynamicStateCount = PS_ARRAY_COUNT(dynamicStates);

    return true;
}

bool psPipelineUtilsInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo *inputAssemblyInfo) {
    inputAssemblyInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo->primitiveRestartEnable = VK_FALSE;

    return true;
}

bool psPipelineUtilsViewportScissor(VkViewport *viewport, VkRect2D *scissor)
{
    PS_ASSERT(viewport != NULL);
    PS_ASSERT(scissor != NULL);

    viewport->x = 0.0f;
    viewport->y = 0.0f;
    viewport->width = 1024.0f;
    viewport->height = 1024.0f;
    viewport->minDepth = 0.0f;
    viewport->maxDepth = 1.0f;

    scissor->offset.x = 0;
    scissor->offset.y = 0;
    scissor->extent.width = 1024;
    scissor->extent.height = 1024;

    return true;
}

bool psPipelineUtilsViewportState(VkPipelineViewportStateCreateInfo *viewportStateInfo, VkViewport *viewport, VkRect2D *scissor) {
    PS_ASSERT(viewportStateInfo != NULL);
    PS_ASSERT(viewport != NULL);
    PS_ASSERT(scissor != NULL);

    viewportStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo->viewportCount = 1;
    viewportStateInfo->pViewports = viewport;
    viewportStateInfo->scissorCount = 1;
    viewportStateInfo->pScissors = scissor;

    return true;
}

bool psPipelineUtilsRasterizationState(VkPipelineRasterizationStateCreateInfo *rasterizerInfo) {
    PS_ASSERT(rasterizerInfo != NULL);

    rasterizerInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo->depthClampEnable = VK_FALSE;
    rasterizerInfo->rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo->polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerInfo->cullMode = VK_CULL_MODE_NONE;
    rasterizerInfo->frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo->depthBiasEnable = VK_FALSE;
    rasterizerInfo->lineWidth = 1.0f;

    return true;
}

bool psPipelineUtilsMultisampleState(VkPipelineMultisampleStateCreateInfo *multisampleInfo) {
    PS_ASSERT(multisampleInfo != NULL);

    multisampleInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo->sampleShadingEnable = VK_FALSE;
    multisampleInfo->minSampleShading = 1.0f;
    multisampleInfo->pSampleMask = NULL;
    multisampleInfo->alphaToCoverageEnable = VK_FALSE;
    multisampleInfo->alphaToOneEnable = VK_FALSE;

    return true;
}

bool psPipelineUtilsDepthStencilState(VkPipelineDepthStencilStateCreateInfo *depthStencilInfo, bool enableDepthTest) {
    PS_ASSERT(depthStencilInfo != NULL);


    depthStencilInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo->depthTestEnable = enableDepthTest ? VK_TRUE : VK_FALSE;
    depthStencilInfo->depthWriteEnable = enableDepthTest ? VK_TRUE : VK_FALSE;
    depthStencilInfo->depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilInfo->depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo->stencilTestEnable = VK_FALSE;

    return true;
}

bool psPipelineUtilsBlendAttachment(VkPipelineColorBlendAttachmentState *blendAttachment, bool enableBlend) {
    PS_ASSERT(blendAttachment != NULL);

    blendAttachment->blendEnable = enableBlend ? VK_TRUE : VK_FALSE;
    blendAttachment->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment->colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachment->srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment->alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    return true;
}

bool psPipelineUtilsColorBlendState(VkPipelineColorBlendStateCreateInfo *colorBlendStateInfo, VkPipelineColorBlendAttachmentState *blendAttachments, size_t attachmentCount) {
    PS_ASSERT(colorBlendStateInfo != NULL);
    PS_ASSERT(blendAttachments != NULL);
    PS_ASSERT(attachmentCount > 0);

    colorBlendStateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo->logicOpEnable = VK_FALSE;
    colorBlendStateInfo->logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateInfo->attachmentCount = (uint32_t)attachmentCount;
    colorBlendStateInfo->pAttachments = blendAttachments;

    return true;
}


bool psWriteImageDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo *imageInfo)
{
    PS_ASSERT(writeDescriptorSet != NULL);
    PS_ASSERT(descriptorSet != VK_NULL_HANDLE);
    PS_ASSERT(imageInfo != NULL);

    writeDescriptorSet->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet->pNext = NULL;
    writeDescriptorSet->dstSet = descriptorSet;
    writeDescriptorSet->dstBinding = binding;
    writeDescriptorSet->dstArrayElement = 0;
    writeDescriptorSet->descriptorCount = 1;
    writeDescriptorSet->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet->pImageInfo = imageInfo;
    writeDescriptorSet->pBufferInfo = NULL;
    writeDescriptorSet->pTexelBufferView = NULL;

    return true;

}

bool psWriteBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo)
{
    PS_ASSERT(writeDescriptorSet != NULL);
    PS_ASSERT(descriptorSet != VK_NULL_HANDLE);
    PS_ASSERT(bufferInfo != NULL);

    writeDescriptorSet->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet->pNext = NULL;
    writeDescriptorSet->dstSet = descriptorSet;
    writeDescriptorSet->dstBinding = binding;
    writeDescriptorSet->dstArrayElement = 0;
    writeDescriptorSet->descriptorCount = 1;
    writeDescriptorSet->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet->pImageInfo = NULL;
    writeDescriptorSet->pBufferInfo = bufferInfo;
    writeDescriptorSet->pTexelBufferView = NULL;

    return true;
}