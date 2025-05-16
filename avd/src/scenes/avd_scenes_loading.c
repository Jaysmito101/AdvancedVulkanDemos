#include "avd_scenes.h"
#include "avd_shader.h"
#include <string.h>

typedef struct PushConstantData {
    float windowWidth;
    float windowHeight;
    float progress;
    float padding;
} PushConstantData;

static bool __avdCreatePipelineLayout(AVD_LoadingScene *scene, VkDevice device) {
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);

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
    AVD_CHECK_VK_RESULT(result, "Failed to create pipeline layout");
    return true;
}

static bool __avdCreatePipeline(AVD_LoadingScene *scene, VkDevice device, VkRenderPass renderPass) {
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = avdShaderModuleCreateFromAsset(device, "LoadingScreenVert");
    AVD_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");
    VkShaderModule fragmentShaderModule = avdShaderModuleCreateFromAsset(device, "LoadingScreenFrag");
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
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics pipeline\n");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool avdScenesLoadingInit(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);
    AVD_LoadingScene *scene = &gameState->scene.loadingScene;
    AVD_CHECK(__avdCreatePipelineLayout(scene, gameState->vulkan.device));
    AVD_CHECK(__avdCreatePipeline(scene, gameState->vulkan.device, gameState->vulkan.renderer.sceneFramebuffer.renderPass));
    return true;
}

void avdScenesLoadingShutdown(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);
    AVD_LoadingScene *scene = &gameState->scene.loadingScene;
    vkDestroyPipeline(gameState->vulkan.device, scene->pipeline, NULL);
    vkDestroyPipelineLayout(gameState->vulkan.device, scene->pipelineLayout, NULL);
}

bool avdScenesLoadingSwitch(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);
    AVD_LoadingScene *scene = &gameState->scene.loadingScene;
    scene->progress = 0.0f;
    scene->sceneStartTime = (float)gameState->framerate.currentTime;
    return true;
}

bool avdScenesLoadingUpdate(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);
    AVD_LoadingScene *scene = &gameState->scene.loadingScene;
    
    double elapsedTime = gameState->framerate.currentTime - scene->sceneStartTime;
    if (elapsedTime < 1.0) return true; // Wait for at least 1 sec for the scene transition to finish

    if(!avdScenesLoadContentScenesAsyncPoll(gameState)) {
        AVD_LOG("Failed to poll content scenes loading... Aborting.\n");
        gameState->running = false;
    }
    scene->progress = gameState->scene.loadingProgress;

    if (gameState->scene.allContentScenesLoaded && !gameState->scene.isSwitchingScene) {
        avdScenesSwitch(gameState, AVD_SCENE_TYPE_MAIN_MENU);
    }
    return true;
}

bool avdScenesLoadingRender(AVD_GameState *gameState) {
    AVD_ASSERT(gameState != NULL);
    AVD_LoadingScene *scene = &gameState->scene.loadingScene;
    uint32_t idx = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer cmd = gameState->vulkan.renderer.resources[idx].commandBuffer;

    AVD_CHECK(avdBeginSceneRenderPass(
        cmd,
        &gameState->vulkan.renderer,
        NULL, 0
    ));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->pipeline);

    PushConstantData pc = {
        .windowWidth = (float)gameState->vulkan.renderer.sceneFramebuffer.width,
        .windowHeight = (float)gameState->vulkan.renderer.sceneFramebuffer.height,
        .progress = scene->progress,
        .padding = 0.0f
    };
    vkCmdPushConstants(cmd, scene->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

    vkCmdDraw(cmd, 6, 1, 0, 0);
    

    AVD_CHECK(avdEndSceneRenderPass(cmd));

    return true;
}
