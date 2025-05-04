#include "ps_vulkan.h"
#include "ps_shader.h"

static bool __psVulkanSceneCreatePipelineLayout(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(float) * 4; // Assuming 4 floats for the push constant range


    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;    
    pipelineLayoutInfo.pushConstantRangeCount = PS_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(gameState->vulkan.device, &pipelineLayoutInfo, NULL, &gameState->vulkan.renderer.scene.pipelineLayout);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create pipeline layout\n");
        return false;
    }

    return true;
}

static bool __psVulkanSceneCreatePipeline(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = gameState->vulkan.renderer.scene.vertexShaderModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = gameState->vulkan.renderer.scene.fragmentShaderModule;
    shaderStages[1].pName = "main";

    VkDynamicState dynamicStates[2] = {0};
    dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {0};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = PS_ARRAY_COUNT(dynamicStates);
    dynamicStateInfo.pDynamicStates = dynamicStates;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;\

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {0};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)gameState->vulkan.swapchain.extent.width;
    viewport.height = (float)gameState->vulkan.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = gameState->vulkan.swapchain.extent;

    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {0};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo.depthBiasEnable = VK_FALSE;
    rasterizerInfo.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {0};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.minSampleShading = 1.0f;
    multisampleInfo.pSampleMask = NULL;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {0};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_FALSE;
    depthStencilInfo.depthWriteEnable = VK_FALSE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.stencilTestEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportStateInfo = {0};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {0};
    colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateInfo.attachmentCount = 1;
    colorBlendStateInfo.pAttachments = &colorBlendAttachment;


    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = PS_ARRAY_COUNT(shaderStages);
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.layout = gameState->vulkan.renderer.scene.pipelineLayout;
    pipelineInfo.renderPass = gameState->vulkan.renderer.scene.framebuffer.renderPass;
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


    VkResult result = vkCreateGraphicsPipelines(gameState->vulkan.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gameState->vulkan.renderer.scene.pipeline);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create graphics pipeline\n");
        return false;
    }

    return true;
}

static bool __psVulkanSceneCreateDescriptors(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[1] = {0};
    descriptorSetLayoutBindings[0].binding = 0;
    descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBindings[0].descriptorCount = 1;
    descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {0};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = PS_ARRAY_COUNT(descriptorSetLayoutBindings);
    descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings;

    VkResult result = vkCreateDescriptorSetLayout(gameState->vulkan.device, &descriptorSetLayoutInfo, NULL, &gameState->vulkan.renderer.scene.framebufferColorDescriptorSetLayout);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create descriptor set layout\n");
        return false;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {0};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = gameState->vulkan.descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &gameState->vulkan.renderer.scene.framebufferColorDescriptorSetLayout;

    result = vkAllocateDescriptorSets(gameState->vulkan.device, &descriptorSetAllocateInfo, &gameState->vulkan.renderer.scene.framebufferColorDescriptorSet);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to allocate descriptor set\n");
        return false;
    }

    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.sampler = gameState->vulkan.renderer.scene.framebuffer.sampler;
    imageInfo.imageView = gameState->vulkan.renderer.scene.framebuffer.colorAttachment.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writeDescriptorSet = {0};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = gameState->vulkan.renderer.scene.framebufferColorDescriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet.pImageInfo = &imageInfo;
    writeDescriptorSet.pBufferInfo = NULL;
    writeDescriptorSet.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(gameState->vulkan.device, 1, &writeDescriptorSet, 0, NULL);

    return true;
}

bool psVulkanSceneInit(PS_GameState *gameState) {
    if (!psVulkanFramebufferCreate(gameState, &gameState->vulkan.renderer.scene.framebuffer, GAME_WIDTH, GAME_HEIGHT, true, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D32_SFLOAT))
    {
        PS_LOG("Failed to create Vulkan framebuffer\n");
        return false;
    }

    if (!__psVulkanSceneCreateDescriptors(gameState)) {
        PS_LOG("Failed to create scene descriptors\n");
        return false;
    }

    VkShaderModule vertexShaderModule = psShaderModuleCreate(gameState, psShader_SceneVertex, VK_SHADER_STAGE_VERTEX_BIT, "scene_vertex_shader.glsl");
    if (vertexShaderModule == VK_NULL_HANDLE) {
        PS_LOG("Failed to create vertex shader module\n");
        return false;
    }
    gameState->vulkan.renderer.scene.vertexShaderModule = vertexShaderModule;

    VkShaderModule fragmentShaderModule = psShaderModuleCreate(gameState, psShader_SceneFragment, VK_SHADER_STAGE_FRAGMENT_BIT, "scene_fragment_shader.glsl");    
    if (fragmentShaderModule == VK_NULL_HANDLE) {
        PS_LOG("Failed to create fragment shader module\n");
        vkDestroyShaderModule(gameState->vulkan.device, vertexShaderModule, NULL);
        return false;
    }
    gameState->vulkan.renderer.scene.fragmentShaderModule = fragmentShaderModule;

    
    if(!__psVulkanSceneCreatePipelineLayout(gameState)) {
        PS_LOG("Failed to create pipeline layout\n");
        return false;
    }

    if (!__psVulkanSceneCreatePipeline(gameState)) {
        PS_LOG("Failed to create scene pipeline\n");
        return false;
    }

    return true;
}

void psVulkanSceneDestroy(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    vkDestroyPipeline(gameState->vulkan.device, gameState->vulkan.renderer.scene.pipeline, NULL);
    gameState->vulkan.renderer.scene.pipeline = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(gameState->vulkan.device, gameState->vulkan.renderer.scene.pipelineLayout, NULL);
    gameState->vulkan.renderer.scene.pipelineLayout = VK_NULL_HANDLE;

    vkDestroyShaderModule(gameState->vulkan.device, gameState->vulkan.renderer.scene.vertexShaderModule, NULL);
    gameState->vulkan.renderer.scene.vertexShaderModule = VK_NULL_HANDLE;

    vkDestroyShaderModule(gameState->vulkan.device, gameState->vulkan.renderer.scene.fragmentShaderModule, NULL);
    gameState->vulkan.renderer.scene.fragmentShaderModule = VK_NULL_HANDLE;

    vkDestroyDescriptorSetLayout(gameState->vulkan.device, gameState->vulkan.renderer.scene.framebufferColorDescriptorSetLayout, NULL);
    gameState->vulkan.renderer.scene.framebufferColorDescriptorSetLayout = VK_NULL_HANDLE;

    psVulkanFramebufferDestroy(gameState, &gameState->vulkan.renderer.scene.framebuffer);
}

bool psVulkanSceneRender(PS_GameState *gameState, uint32_t imageIndex)
{
    PS_ASSERT(gameState != NULL);
    uint32_t currentFrameIndex = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer commandBuffer = gameState->vulkan.renderer.resources[currentFrameIndex].commandBuffer;

    static VkClearValue clearColor[2] = {0};
    clearColor[0].color.float32[0] = 0.1f;
    clearColor[0].color.float32[1] = 0.1f;
    clearColor[0].color.float32[2] = 0.1f;
    clearColor[0].color.float32[3] = 1.0f;

    clearColor[1].depthStencil.depth = 1.0f;
    clearColor[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = gameState->vulkan.renderer.scene.framebuffer.renderPass;
    renderPassInfo.framebuffer = gameState->vulkan.renderer.scene.framebuffer.framebuffer;
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent.width = gameState->vulkan.renderer.scene.framebuffer.width;
    renderPassInfo.renderArea.extent.height = gameState->vulkan.renderer.scene.framebuffer.height;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)gameState->vulkan.renderer.scene.framebuffer.width;
    viewport.height = (float)gameState->vulkan.renderer.scene.framebuffer.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = gameState->vulkan.renderer.scene.framebuffer.width;
    scissor.extent.height = gameState->vulkan.renderer.scene.framebuffer.height;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    const float pushConstantData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameState->vulkan.renderer.scene.pipeline);
    vkCmdPushConstants(commandBuffer, gameState->vulkan.renderer.scene.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstantData), pushConstantData);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    

    vkCmdEndRenderPass(commandBuffer);

    return true;
}