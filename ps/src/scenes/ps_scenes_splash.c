#include "ps_scenes.h"
#include "ps_shader.h"

typedef struct PS_SplashPushConstants
{
    float framebufferWidth;
    float framebufferHeight;
    float imageWidth;
    float imageHeight;
    float scale;
    float opacity;
} PS_SplashPushConstants;

static bool __psCreateDescriptorSetLayout(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    VkDescriptorSetLayoutBinding layoutBinding = {0};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(gameState->vulkan.device, &layoutInfo, NULL,
                                    &gameState->scene.splashScene.descriptorSetLayout) != VK_SUCCESS)
    {
        PS_LOG("Failed to create descriptor set layout\n");
        return false;
    }

    return true;
}

static bool __psCreateDescriptorSet(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = gameState->vulkan.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &gameState->scene.splashScene.descriptorSetLayout;

    if (vkAllocateDescriptorSets(gameState->vulkan.device, &allocInfo,
                                 &gameState->scene.splashScene.descriptorSet) != VK_SUCCESS)
    {
        PS_LOG("Failed to allocate descriptor set\n");
        return false;
    }

    VkWriteDescriptorSet descriptorWrite = {0};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = gameState->scene.splashScene.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &gameState->scene.splashScene.splashImage.descriptorImageInfo;
    vkUpdateDescriptorSets(gameState->vulkan.device, 1, &descriptorWrite, 0, NULL);

    return true;
}

static bool __psCreatePipelineLayout(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(PS_SplashPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &gameState->scene.splashScene.descriptorSetLayout;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    pipelineLayoutInfo.pushConstantRangeCount = PS_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(gameState->vulkan.device, &pipelineLayoutInfo, NULL, &gameState->scene.splashScene.pipelineLayout);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create pipeline layout\n");
        return false;
    }

    return true;
}

static bool __psCreatePipeline(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = gameState->scene.splashScene.vertexShaderModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = gameState->scene.splashScene.fragmentShaderModule;
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
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
    pipelineInfo.layout = gameState->scene.splashScene.pipelineLayout;
    pipelineInfo.renderPass = gameState->vulkan.renderer.sceneFramebuffer.renderPass;
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

    VkResult result = vkCreateGraphicsPipelines(gameState->vulkan.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gameState->scene.splashScene.pipeline);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create graphics pipeline\n");
        return false;
    }

    return true;
}

bool psScenesSplashInit(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    // Load splash image
    if (!psVulkanImageLoadFromFile(gameState, "./assets/splash.png", &gameState->scene.splashScene.splashImage))
    {
        PS_LOG("Failed to load splash image\n");
        return false;
    }

    // Create descriptor set layout
    if (!__psCreateDescriptorSetLayout(gameState))
    {
        PS_LOG("Failed to create descriptor set layout\n");
        return false;
    }

    // Create descriptor set
    if (!__psCreateDescriptorSet(gameState))
    {
        PS_LOG("Failed to create descriptor set\n");
        return false;
    }

    VkShaderModule vertexShaderModule = psShaderModuleCreate(gameState, psShader_SplashSceneVertex, VK_SHADER_STAGE_VERTEX_BIT, "splash_scene_vertex_shader.glsl");
    if (vertexShaderModule == VK_NULL_HANDLE)
    {
        PS_LOG("Failed to create vertex shader module\n");
        return false;
    }
    gameState->scene.splashScene.vertexShaderModule = vertexShaderModule;

    VkShaderModule fragmentShaderModule = psShaderModuleCreate(gameState, psShader_SplashSceneFragment, VK_SHADER_STAGE_FRAGMENT_BIT, "splash_scene_fragment_shader.glsl");
    if (fragmentShaderModule == VK_NULL_HANDLE)
    {
        PS_LOG("Failed to create fragment shader module\n");
        vkDestroyShaderModule(gameState->vulkan.device, vertexShaderModule, NULL);
        return false;
    }
    gameState->scene.splashScene.fragmentShaderModule = fragmentShaderModule;

    if (!__psCreatePipelineLayout(gameState))
    {
        PS_LOG("Failed to create pipeline layout\n");
        return false;
    }

    if (!__psCreatePipeline(gameState))
    {
        PS_LOG("Failed to create scene pipeline\n");
        return false;
    }

    return true;
}

void psScenesSplashShutdown(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    // Clean up image resources
    psVulkanImageDestroy(gameState, &gameState->scene.splashScene.splashImage);

    // Destroy descriptor resources
    vkDestroyDescriptorSetLayout(gameState->vulkan.device, gameState->scene.splashScene.descriptorSetLayout, NULL);

    vkDestroyPipeline(gameState->vulkan.device, gameState->scene.splashScene.pipeline, NULL);
    gameState->scene.splashScene.pipeline = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(gameState->vulkan.device, gameState->scene.splashScene.pipelineLayout, NULL);
    gameState->scene.splashScene.pipelineLayout = VK_NULL_HANDLE;

    vkDestroyShaderModule(gameState->vulkan.device, gameState->scene.splashScene.vertexShaderModule, NULL);
    gameState->scene.splashScene.vertexShaderModule = VK_NULL_HANDLE;

    vkDestroyShaderModule(gameState->vulkan.device, gameState->scene.splashScene.fragmentShaderModule, NULL);
    gameState->scene.splashScene.fragmentShaderModule = VK_NULL_HANDLE;
}

bool psScenesSplashSwitch(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    gameState->scene.currentScene = PS_SCENE_TYPE_SPLASH;
    gameState->scene.splashScene.sceneStartTime = gameState->framerate.currentTime;
    gameState->scene.splashScene.sceneDurationLeft = 5.0; // 5 seconds duration

    return true;
}

bool psScenesSplashRender(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    uint32_t currentFrameIndex = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer commandBuffer = gameState->vulkan.renderer.resources[currentFrameIndex].commandBuffer;

    static VkClearValue clearColor[2] = {0};
    clearColor[0].color.float32[0] = 215.0f / 255.0f;
    clearColor[0].color.float32[1] = 204.0f / 255.0f;
    clearColor[0].color.float32[2] = 246.0f / 255.0f;
    clearColor[0].color.float32[3] = 1.0f;

    clearColor[1].depthStencil.depth = 1.0f;
    clearColor[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = gameState->vulkan.renderer.sceneFramebuffer.renderPass;
    renderPassInfo.framebuffer = gameState->vulkan.renderer.sceneFramebuffer.framebuffer;
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent.width = gameState->vulkan.renderer.sceneFramebuffer.width;
    renderPassInfo.renderArea.extent.height = gameState->vulkan.renderer.sceneFramebuffer.height;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)gameState->vulkan.renderer.sceneFramebuffer.width;
    viewport.height = (float)gameState->vulkan.renderer.sceneFramebuffer.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = gameState->vulkan.renderer.sceneFramebuffer.width;
    scissor.extent.height = gameState->vulkan.renderer.sceneFramebuffer.height;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Populate the push constant struct
    PS_SplashPushConstants pushConstantData = {
        .framebufferWidth = (float)gameState->vulkan.renderer.sceneFramebuffer.width,
        .framebufferHeight = (float)gameState->vulkan.renderer.sceneFramebuffer.height,
        .imageWidth = (float)gameState->scene.splashScene.splashImage.width,
        .imageHeight = (float)gameState->scene.splashScene.splashImage.height,
        .scale = 0.4f,         // Example scale
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameState->scene.splashScene.pipeline);
    // Push the struct
    vkCmdPushConstants(
        commandBuffer,
        gameState->scene.splashScene.pipelineLayout,
        // Ensure flags cover all stages using the constants
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(PS_SplashPushConstants), // Use struct size
        &pushConstantData                  // Pass struct address
    );

    // Bind the texture descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            gameState->scene.splashScene.pipelineLayout, 0, 1,
                            &gameState->scene.splashScene.descriptorSet, 0, NULL);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    return true;
}

bool psScenesSplashUpdate(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    return true;
}
