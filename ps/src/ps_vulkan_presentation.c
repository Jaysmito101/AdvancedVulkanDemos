#include "ps_vulkan.h"
#include "ps_shader.h"

static bool __psVulkanPresentationCreatePipelineLayout(PS_GameState *gameState) {
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

    VkResult result = vkCreatePipelineLayout(gameState->vulkan.device, &pipelineLayoutInfo, NULL, &gameState->vulkan.renderer.presentation.pipelineLayout);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create pipeline layout\n");
        return false;
    }

    return true;
}

static bool __psVulkanPresentationCreatePipeline(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = gameState->vulkan.renderer.presentation.vertexShaderModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = gameState->vulkan.renderer.presentation.fragmentShaderModule;
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
    pipelineInfo.layout = gameState->vulkan.renderer.presentation.pipelineLayout;
    pipelineInfo.renderPass = gameState->vulkan.swapchain.renderPass;
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


    VkResult result = vkCreateGraphicsPipelines(gameState->vulkan.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gameState->vulkan.renderer.presentation.pipeline);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create graphics pipeline\n");
        return false;
    }

    return true;
}

bool psVulkanPresentationInit(PS_GameState *gameState) {
    VkShaderModule vertexShaderModule = psShaderModuleCreate(gameState, psShader_PresentationVertex, VK_SHADER_STAGE_VERTEX_BIT, "presentation_vertex_shader.glsl");
    if (vertexShaderModule == VK_NULL_HANDLE) {
        PS_LOG("Failed to create vertex shader module\n");
        return false;
    }
    gameState->vulkan.renderer.presentation.vertexShaderModule = vertexShaderModule;

    VkShaderModule fragmentShaderModule = psShaderModuleCreate(gameState, psShader_PresentationFragment, VK_SHADER_STAGE_FRAGMENT_BIT, "presentation_fragment_shader.glsl");    
    if (fragmentShaderModule == VK_NULL_HANDLE) {
        PS_LOG("Failed to create fragment shader module\n");
        vkDestroyShaderModule(gameState->vulkan.device, vertexShaderModule, NULL);
        return false;
    }
    gameState->vulkan.renderer.presentation.fragmentShaderModule = fragmentShaderModule;
    
    if(!__psVulkanPresentationCreatePipelineLayout(gameState)) {
        PS_LOG("Failed to create pipeline layout\n");
        return false;
    }

    if (!__psVulkanPresentationCreatePipeline(gameState)) {
        PS_LOG("Failed to create presentation pipeline\n");
        return false;
    }

    return true;
}

void psVulkanPresentationDestroy(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    vkDestroyPipeline(gameState->vulkan.device, gameState->vulkan.renderer.presentation.pipeline, NULL);
    gameState->vulkan.renderer.presentation.pipeline = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(gameState->vulkan.device, gameState->vulkan.renderer.presentation.pipelineLayout, NULL);
    gameState->vulkan.renderer.presentation.pipelineLayout = VK_NULL_HANDLE;

    vkDestroyShaderModule(gameState->vulkan.device, gameState->vulkan.renderer.presentation.vertexShaderModule, NULL);
    gameState->vulkan.renderer.presentation.vertexShaderModule = VK_NULL_HANDLE;

    vkDestroyShaderModule(gameState->vulkan.device, gameState->vulkan.renderer.presentation.fragmentShaderModule, NULL);
    gameState->vulkan.renderer.presentation.fragmentShaderModule = VK_NULL_HANDLE;
}

bool psVulkanPresentationRender(PS_GameState *gameState, uint32_t imageIndex)
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
    renderPassInfo.renderPass = gameState->vulkan.swapchain.renderPass;
    renderPassInfo.framebuffer = gameState->vulkan.swapchain.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = gameState->vulkan.swapchain.extent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)gameState->vulkan.swapchain.extent.width;
    viewport.height = (float)gameState->vulkan.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = gameState->vulkan.swapchain.extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    const float pushConstantData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameState->vulkan.renderer.presentation.pipeline);
    vkCmdPushConstants(commandBuffer, gameState->vulkan.renderer.presentation.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstantData), pushConstantData);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    

    vkCmdEndRenderPass(commandBuffer);

    return true;
}