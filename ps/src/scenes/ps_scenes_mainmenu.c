#include "ps_scenes.h"
#include "ps_shader.h"

// Push constant struct matching shader data
typedef struct PS_MainMenuPushConstants {
    float windowWidth;
    float windowHeight;
    float time;
} PS_MainMenuPushConstants;

bool psScenesMainMenuInit(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;

    // Create shader modules
    VkShaderModule vert = psShaderModuleCreate(gameState, psShader_MainMenuVertex, VK_SHADER_STAGE_VERTEX_BIT, "main_menu_vertex_shader.glsl");
    if (vert == VK_NULL_HANDLE) return false;
    scene->vertexShaderModule = vert;

    VkShaderModule frag = psShaderModuleCreate(gameState, psShader_MainMenuFragment, VK_SHADER_STAGE_FRAGMENT_BIT, "main_menu_fragment_shader.glsl");
    if (frag == VK_NULL_HANDLE) {
        vkDestroyShaderModule(gameState->vulkan.device, vert, NULL);
        return false;
    }
    scene->fragmentShaderModule = frag;

    // Pipeline layout (push constants only)
    VkPushConstantRange pushRange = {0};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PS_MainMenuPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pSetLayouts = NULL;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    if (vkCreatePipelineLayout(gameState->vulkan.device, &layoutInfo, NULL, &scene->pipelineLayout) != VK_SUCCESS) {
        PS_LOG("Failed to create main menu pipeline layout\n");
        return false;
    }

    // Graphics pipeline (Similar to loading screen, no vertex input)
    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = scene->vertexShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = scene->fragmentShaderModule;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInput = {0};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 0;
    vertexInput.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {0}; // Dynamic state
    VkRect2D scissor = {0}; // Dynamic state

    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport; // Dynamic
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor; // Dynamic

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample = {0};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendState = {0};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = PS_ARRAY_COUNT(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = scene->pipelineLayout;
    pipelineInfo.renderPass = gameState->vulkan.renderer.sceneFramebuffer.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(gameState->vulkan.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &scene->pipeline) != VK_SUCCESS) {
        PS_LOG("Failed to create main menu graphics pipeline\n");
        return false;
    }

    return true;
}

void psScenesMainMenuShutdown(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;
    vkDestroyPipeline(gameState->vulkan.device, scene->pipeline, NULL);
    vkDestroyPipelineLayout(gameState->vulkan.device, scene->pipelineLayout, NULL);
    vkDestroyShaderModule(gameState->vulkan.device, scene->vertexShaderModule, NULL);
    vkDestroyShaderModule(gameState->vulkan.device, scene->fragmentShaderModule, NULL);
}

bool psScenesMainMenuSwitch(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    return true;
}

bool psScenesMainMenuUpdate(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    return true;
}

bool psScenesMainMenuRender(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;
    uint32_t idx = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer cmd = gameState->vulkan.renderer.resources[idx].commandBuffer;

    // Begin scene render pass (clears to dark grey)
    VkRenderPassBeginInfo rpInfo = {0};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = gameState->vulkan.renderer.sceneFramebuffer.renderPass;
    rpInfo.framebuffer = gameState->vulkan.renderer.sceneFramebuffer.framebuffer;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent.width = gameState->vulkan.renderer.sceneFramebuffer.width;
    rpInfo.renderArea.extent.height = gameState->vulkan.renderer.sceneFramebuffer.height;
    VkClearValue clears[2] = {0};
    clears[0].color.float32[0] = 40.0f/255.0f;
    clears[0].color.float32[1] = 40.0f/255.0f;
    clears[0].color.float32[2] = 40.0f/255.0f;
    clears[0].color.float32[3] = 1.0f;
    clears[1].depthStencil.depth = 1.0f;
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clears;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)gameState->vulkan.renderer.sceneFramebuffer.width;
    viewport.height = (float)gameState->vulkan.renderer.sceneFramebuffer.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = gameState->vulkan.renderer.sceneFramebuffer.width;
    scissor.extent.height = gameState->vulkan.renderer.sceneFramebuffer.height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->pipeline);

    // Push constants
    PS_MainMenuPushConstants pc = {
        .windowWidth = (float)gameState->window.framebufferWidth, // Use framebuffer size
        .windowHeight = (float)gameState->window.framebufferHeight,
        .time = (float)gameState->framerate.currentTime
    };
    vkCmdPushConstants(cmd, scene->pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pc), &pc);

    // Draw full screen quad (6 vertices)
    vkCmdDraw(cmd, 6, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
    return true;
}
