#include "ps_scenes.h"
#include "ps_shader.h"
#include <string.h>

typedef struct PushConstantData {
    float windowWidth;
    float windowHeight;
    float progress;
    float padding;
} PushConstantData;

static bool __psCreatePipelineLayout(PS_LoadingScene *scene, VkDevice device) {
    PS_ASSERT(scene != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);

    VkPushConstantRange pushConstantRange = {0};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);

    VkPipelineLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pSetLayouts = NULL;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(device, &layoutInfo, NULL, &scene->pipelineLayout);
    PS_CHECK_VK_RESULT(result, "Failed to create pipeline layout");
    return true;
}

static bool __psCreatePipeline(PS_LoadingScene *scene, VkDevice device, VkRenderPass renderPass) {
    PS_ASSERT(scene != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = psShaderModuleCreateFromAsset(device, "LoadingScreenVert");
    PS_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");
    VkShaderModule fragmentShaderModule = psShaderModuleCreateFromAsset(device, "LoadingScreenFrag");
    PS_CHECK_VK_HANDLE(fragmentShaderModule, "Failed to create fragment shader module\n");

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    PS_CHECK(psPipelineUtilsShaderStage(&shaderStages[0], vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT));
    PS_CHECK(psPipelineUtilsShaderStage(&shaderStages[1], fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT));

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {0};
    PS_CHECK(psPipelineUtilsDynamicState(&dynamicStateInfo));

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {0};
    PS_CHECK(psPipelineUtilsInputAssemblyState(&inputAssemblyInfo));

    VkViewport viewport = {0};
    VkRect2D scissor = {0};
    PS_CHECK(psPipelineUtilsViewportScissor(&viewport, &scissor));
    
    VkPipelineViewportStateCreateInfo viewportStateInfo = {0};
    PS_CHECK(psPipelineUtilsViewportState(&viewportStateInfo, &viewport, &scissor));

    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {0};
    PS_CHECK(psPipelineUtilsRasterizationState(&rasterizerInfo));
    
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {0};
    PS_CHECK(psPipelineUtilsMultisampleState(&multisampleInfo));
    
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {0};
    PS_CHECK(psPipelineUtilsDepthStencilState(&depthStencilInfo, false));

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    PS_CHECK(psPipelineUtilsBlendAttachment(&colorBlendAttachment, true));

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {0};
    PS_CHECK(psPipelineUtilsColorBlendState(&colorBlendStateInfo, &colorBlendAttachment, 1));

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = PS_ARRAY_COUNT(shaderStages);
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendStateInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = scene->pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &scene->pipeline);
    PS_CHECK_VK_RESULT(result, "Failed to create graphics pipeline\n");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool psScenesLoadingInit(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_LoadingScene *scene = &gameState->scene.loadingScene;
    PS_CHECK(__psCreatePipelineLayout(scene, gameState->vulkan.device));
    PS_CHECK(__psCreatePipeline(scene, gameState->vulkan.device, gameState->vulkan.renderer.sceneFramebuffer.renderPass));
    return true;
}

void psScenesLoadingShutdown(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_LoadingScene *scene = &gameState->scene.loadingScene;
    vkDestroyPipeline(gameState->vulkan.device, scene->pipeline, NULL);
    vkDestroyPipelineLayout(gameState->vulkan.device, scene->pipelineLayout, NULL);
}

bool psScenesLoadingSwitch(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_LoadingScene *scene = &gameState->scene.loadingScene;
    scene->progress = 0.0f;
    scene->sceneStartTime = (float)gameState->framerate.currentTime;
    return true;
}

bool psScenesLoadingUpdate(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_LoadingScene *scene = &gameState->scene.loadingScene;
    
    double elapsedTime = gameState->framerate.currentTime - scene->sceneStartTime;
    if (elapsedTime < 1.0) return true; // Wait for at least 1 sec for the scene transition to finish

    if(!psScenesLoadContentScenesAsyncPoll(gameState)) {
        PS_LOG("Failed to poll content scenes loading... Aborting.\n");
        gameState->running = false;
    }
    scene->progress = gameState->scene.loadingProgress;

    if (gameState->scene.allContentScenesLoaded && !gameState->scene.isSwitchingScene) {
        psScenesSwitch(gameState, PS_SCENE_TYPE_MAIN_MENU);
    }
    return true;
}

bool psScenesLoadingRender(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_LoadingScene *scene = &gameState->scene.loadingScene;
    uint32_t idx = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer cmd = gameState->vulkan.renderer.resources[idx].commandBuffer;

    VkRenderPassBeginInfo rpInfo = {0};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = gameState->vulkan.renderer.sceneFramebuffer.renderPass;
    rpInfo.framebuffer = gameState->vulkan.renderer.sceneFramebuffer.framebuffer;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent.width = gameState->vulkan.renderer.sceneFramebuffer.width;
    rpInfo.renderArea.extent.height = gameState->vulkan.renderer.sceneFramebuffer.height;
    VkClearValue clears[2] = {0};
    // Update clear color to match the desired background rgb(208, 166, 228)
    clears[0].color.float32[0] = 208.0f / 255.0f;
    clears[0].color.float32[1] = 166.0f / 255.0f;
    clears[0].color.float32[2] = 228.0f / 255.0f;
    clears[0].color.float32[3] = 1.0f;
    clears[1].depthStencil.depth = 1.0f;
    clears[1].depthStencil.stencil = 0;
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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->pipeline);

    PushConstantData pc = {
        .windowWidth = (float)gameState->vulkan.renderer.sceneFramebuffer.width,
        .windowHeight = (float)gameState->vulkan.renderer.sceneFramebuffer.height,
        .progress = scene->progress,
        .padding = 0.0f
    };
    vkCmdPushConstants(cmd, scene->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

    vkCmdDraw(cmd, 6, 1, 0, 0);
    vkCmdEndRenderPass(cmd);
    return true;
}
