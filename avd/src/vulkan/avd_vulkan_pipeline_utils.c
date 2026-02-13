#include "vulkan/avd_vulkan_pipeline_utils.h"
#include "shader/avd_shader.h"
#include "vulkan/avd_vulkan_renderer.h"

bool avdPipelineUtilsShaderStage(VkPipelineShaderStageCreateInfo *shaderStageInfo, VkShaderModule shaderModule, VkShaderStageFlagBits stageFlags)
{
    AVD_ASSERT(shaderStageInfo != NULL);
    AVD_ASSERT(shaderModule != VK_NULL_HANDLE);

    *shaderStageInfo = (VkPipelineShaderStageCreateInfo){
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = NULL,
        .flags               = 0,
        .stage               = stageFlags,
        .module              = shaderModule,
        .pName               = "main",
        .pSpecializationInfo = NULL,
    };

    return true;
}

bool avdPipelineUtilsDynamicState(VkPipelineDynamicStateCreateInfo *dynamicStateInfo)
{
    AVD_ASSERT(dynamicStateInfo != NULL);

    static VkDynamicState dynamicStates[2] = {0};
    dynamicStates[0]                       = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStates[1]                       = VK_DYNAMIC_STATE_SCISSOR;

    *dynamicStateInfo = (VkPipelineDynamicStateCreateInfo){
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = NULL,
        .flags             = 0,
        .pDynamicStates    = dynamicStates,
        .dynamicStateCount = AVD_ARRAY_COUNT(dynamicStates),
    };

    return true;
}

bool avdPipelineUtilsInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo *inputAssemblyInfo)
{
    *inputAssemblyInfo = (VkPipelineInputAssemblyStateCreateInfo){
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = NULL,
        .flags                  = 0,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    return true;
}

bool avdPipelineUtilsViewportScissor(VkViewport *viewport, VkRect2D *scissor)
{
    AVD_ASSERT(viewport != NULL);
    AVD_ASSERT(scissor != NULL);
    *viewport = (VkViewport){
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = 1024.0f,
        .height   = 1024.0f,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    *scissor = (VkRect2D){
        .offset = {.x = 0, .y = 0},
        .extent = {.width = 1024, .height = 1024},
    };

    return true;
}

bool avdPipelineUtilsViewportState(VkPipelineViewportStateCreateInfo *viewportStateInfo, VkViewport *viewport, VkRect2D *scissor)
{
    AVD_ASSERT(viewportStateInfo != NULL);
    AVD_ASSERT(viewport != NULL);
    AVD_ASSERT(scissor != NULL);

    *viewportStateInfo = (VkPipelineViewportStateCreateInfo){
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = NULL,
        .flags         = 0,
        .viewportCount = 1,
        .pViewports    = viewport,
        .scissorCount  = 1,
        .pScissors     = scissor,
    };

    return true;
}

bool avdPipelineUtilsRasterizationState(VkPipelineRasterizationStateCreateInfo *rasterizerInfo, AVD_VulkanPipelineCreationInfo *creationInfo)
{
    AVD_ASSERT(rasterizerInfo != NULL);
    VkPolygonMode polygonMode = creationInfo ? creationInfo->polygonMode : VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode  = creationInfo ? creationInfo->cullMode : VK_CULL_MODE_NONE;
    VkFrontFace frontFace     = creationInfo ? creationInfo->frontFace : VK_FRONT_FACE_CLOCKWISE;

    *rasterizerInfo = (VkPipelineRasterizationStateCreateInfo){
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = NULL,
        .flags                   = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = polygonMode,
        .cullMode                = cullMode,
        .frontFace               = frontFace,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f,
    };

    return true;
}

bool avdPipelineUtilsMultisampleState(VkPipelineMultisampleStateCreateInfo *multisampleInfo)
{
    AVD_ASSERT(multisampleInfo != NULL);
    *multisampleInfo = (VkPipelineMultisampleStateCreateInfo){
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = NULL,
        .flags                 = 0,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    return true;
}

bool avdPipelineUtilsDepthStencilState(VkPipelineDepthStencilStateCreateInfo *depthStencilInfo, bool enableDepthTest)
{
    AVD_ASSERT(depthStencilInfo != NULL);
    *depthStencilInfo = (VkPipelineDepthStencilStateCreateInfo){
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = NULL,
        .flags                 = 0,
        .depthTestEnable       = enableDepthTest ? VK_TRUE : VK_FALSE,
        .depthWriteEnable      = enableDepthTest ? VK_TRUE : VK_FALSE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
        .front                 = {0},
        .back                  = {0},
    };

    return true;
}

bool avdPipelineUtilsBlendAttachment(VkPipelineColorBlendAttachmentState *blendAttachment, bool enableBlend)
{
    AVD_ASSERT(blendAttachment != NULL);
    *blendAttachment = (VkPipelineColorBlendAttachmentState){
        .blendEnable         = enableBlend ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    return true;
}

bool avdPipelineUtilsColorBlendState(VkPipelineColorBlendStateCreateInfo *colorBlendStateInfo, VkPipelineColorBlendAttachmentState *blendAttachments, size_t attachmentCount)
{
    AVD_ASSERT(colorBlendStateInfo != NULL);
    AVD_ASSERT(blendAttachments != NULL);
    *colorBlendStateInfo = (VkPipelineColorBlendStateCreateInfo){
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = NULL,
        .flags           = 0,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = (uint32_t)attachmentCount,
        .pAttachments    = blendAttachments,
        .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    return true;
}

void avdPipelineUtilsPipelineCreationInfoInit(AVD_VulkanPipelineCreationInfo *info)
{
    AVD_ASSERT(info != NULL);

    *info = (AVD_VulkanPipelineCreationInfo){
        .enableBlend     = false,
        .enableDepthTest = false,
        .polygonMode     = VK_POLYGON_MODE_FILL,
        .cullMode        = VK_CULL_MODE_NONE,
        .frontFace       = VK_FRONT_FACE_CLOCKWISE,
    };
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

    VkPushConstantRange pushConstantRanges[1] = {
        [0] = {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
               .offset     = 0,
               .size       = pushConstantSize},
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = (uint32_t)descriptorSetLayoutCount,
        .pSetLayouts            = descriptorSetLayouts,
        .pPushConstantRanges    = pushConstantRanges,
        .pushConstantRangeCount = AVD_ARRAY_COUNT(pushConstantRanges),
    };

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, pipelineLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create pipeline layout for presentation");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        *pipelineLayout,
        "[PipelineLayout][Core]:Vulkan/Pipeline/Layout");

    return true;
}

bool avdPipelineUtilsCreateGenericGraphicsPipeline(
    VkPipeline *pipeline,
    VkPipelineLayout layout,
    VkDevice device,
    VkRenderPass renderPass,
    uint32_t attachmentCount,
    const char *vertShaderAsset,
    const char *fragShaderAsset,
    AVD_ShaderCompilationOptions *compilationOptions,
    AVD_VulkanPipelineCreationInfo *creationInfo)
{
    AVD_ASSERT(pipeline != NULL);
    AVD_ASSERT(layout != VK_NULL_HANDLE);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);
    AVD_ASSERT(vertShaderAsset != NULL);
    AVD_ASSERT(fragShaderAsset != NULL);

    VkShaderModule vertexShaderModule, fragmentShaderModule;
    AVD_CHECK(avdShaderModuleCreate(device, vertShaderAsset, compilationOptions, &vertexShaderModule));
    AVD_CHECK(avdShaderModuleCreate(device, fragShaderAsset, compilationOptions, &fragmentShaderModule));

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
    AVD_CHECK(avdPipelineUtilsRasterizationState(&rasterizerInfo, creationInfo));

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {0};
    AVD_CHECK(avdPipelineUtilsMultisampleState(&multisampleInfo));

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {0};
    AVD_CHECK(avdPipelineUtilsDepthStencilState(&depthStencilInfo, creationInfo ? creationInfo->enableDepthTest : false));

    VkPipelineColorBlendAttachmentState colorBlendAttachment[64] = {0};
    for (uint32_t i = 0; i < attachmentCount; ++i) {
        AVD_CHECK(avdPipelineUtilsBlendAttachment(&colorBlendAttachment[i], creationInfo ? creationInfo->enableBlend : true));
    }

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {0};
    AVD_CHECK(avdPipelineUtilsColorBlendState(&colorBlendStateInfo, colorBlendAttachment, attachmentCount));

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = AVD_ARRAY_COUNT(shaderStages),
        .pStages             = shaderStages,
        .layout              = layout,
        .renderPass          = renderPass,
        .subpass             = 0,
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState      = &viewportStateInfo,
        .pRasterizationState = &rasterizerInfo,
        .pMultisampleState   = &multisampleInfo,
        .pDepthStencilState  = &depthStencilInfo,
        .pColorBlendState    = &colorBlendStateInfo,
        .pDynamicState       = &dynamicStateInfo,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics pipeline for Ui");
    AVD_DEBUG_VK_SET_OBJECT_NAME(
        VK_OBJECT_TYPE_PIPELINE,
        *pipeline,
        "[Pipeline][Core]:Vulkan/Pipeline/Graphics/%s/%s",
        vertShaderAsset,
        fragShaderAsset);

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
    VkPipelineLayout *pipelineLayout,
    VkPipeline *pipeline,
    VkDevice device,
    VkDescriptorSetLayout *descriptorSetLayouts,
    size_t descriptorSetLayoutCount,
    uint32_t pushConstantSize,
    VkRenderPass renderPass,
    uint32_t attachmentCount,
    const char *vertShaderAsset,
    const char *fragShaderAsset,
    AVD_ShaderCompilationOptions *compilationOptions,
    AVD_VulkanPipelineCreationInfo *pipelineCreationInfo)
{
    AVD_ASSERT(pipelineLayout != NULL);
    AVD_ASSERT(pipeline != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);
    AVD_ASSERT(vertShaderAsset != NULL);
    AVD_ASSERT(fragShaderAsset != NULL);

    AVD_CHECK(avdPipelineUtilsCreateGraphicsPipelineLayout(
        pipelineLayout,
        device,
        descriptorSetLayouts,
        descriptorSetLayoutCount,
        pushConstantSize));
    AVD_CHECK(avdPipelineUtilsCreateGenericGraphicsPipeline(
        pipeline,
        *pipelineLayout,
        device,
        renderPass,
        attachmentCount,
        vertShaderAsset,
        fragShaderAsset,
        compilationOptions,
        pipelineCreationInfo));

    return true;
}

bool avdWriteImageDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo *imageInfo)
{
    AVD_ASSERT(writeDescriptorSet != NULL);
    AVD_ASSERT(descriptorSet != VK_NULL_HANDLE);
    AVD_ASSERT(imageInfo != NULL);
    *writeDescriptorSet = (VkWriteDescriptorSet){
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = NULL,
        .dstSet           = descriptorSet,
        .dstBinding       = binding,
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo       = imageInfo,
        .pBufferInfo      = NULL,
        .pTexelBufferView = NULL,
    };

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

bool avdAllocateDescriptorSet(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout,
    VkDescriptorSet *descriptorSet)
{
    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &descriptorSetLayout,
    };
    AVD_CHECK_VK_RESULT(vkAllocateDescriptorSets(
                            device,
                            &allocateInfo,
                            descriptorSet),
                        "Failed to allocate descriptor set");
    return true;
}

bool avdWriteBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo)
{
    AVD_ASSERT(writeDescriptorSet != NULL);
    AVD_ASSERT(descriptorSet != VK_NULL_HANDLE);
    AVD_ASSERT(bufferInfo != NULL);
    *writeDescriptorSet = (VkWriteDescriptorSet){
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = NULL,
        .dstSet           = descriptorSet,
        .dstBinding       = binding,
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo       = NULL,
        .pBufferInfo      = bufferInfo,
        .pTexelBufferView = NULL,
    };

    return true;
}

bool avdWriteUniformBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo)
{
    AVD_ASSERT(writeDescriptorSet != NULL);
    AVD_ASSERT(descriptorSet != VK_NULL_HANDLE);
    AVD_ASSERT(bufferInfo != NULL);
    *writeDescriptorSet = (VkWriteDescriptorSet){
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = NULL,
        .dstSet           = descriptorSet,
        .dstBinding       = binding,
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo       = NULL,
        .pBufferInfo      = bufferInfo,
        .pTexelBufferView = NULL,
    };

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

    VkRenderPassAttachmentBeginInfo attachmentBeginInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO,
        .attachmentCount = (uint32_t)attachmentCount,
        .pAttachments    = attachments,
    };

    VkRenderPassBeginInfo renderPassInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass      = renderPass,
        .framebuffer     = framebuffer,
        .renderArea      = {.offset = {0, 0}, .extent = {framebufferWidth, framebufferHeight}},
        .clearValueCount = (uint32_t)clearValueCount,
        .pClearValues    = clearValues,
        .pNext           = &attachmentBeginInfo,
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float)framebufferWidth,
        .height   = (float)framebufferHeight,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {.x = 0, .y = 0},
        .extent = {.width = framebufferWidth, .height = framebufferHeight},
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    return true;
}

bool avdEndRenderPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
    return true;
}

bool avdBeginRenderPassWithFramebuffer(VkCommandBuffer commandBuffer, AVD_VulkanFramebuffer *framebuffer, VkClearValue *customClearValues, size_t customClearValueCount)
{
    AVD_ASSERT(framebuffer != NULL);
    static VkImageView attachments[16] = {0};
    static size_t attachmentCount      = 0;

    AVD_CHECK(avdVulkanFramebufferGetAttachmentViews(
        framebuffer,
        attachments,
        &attachmentCount));

    AVD_CHECK(avdBeginRenderPass(
        commandBuffer,
        framebuffer->renderPass,
        framebuffer->framebuffer,
        attachments,
        attachmentCount,
        framebuffer->width,
        framebuffer->height,
        customClearValues,
        customClearValueCount));

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