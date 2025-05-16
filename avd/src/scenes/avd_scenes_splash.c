#include "avd_scenes.h"
#include "avd_shader.h"

typedef struct AVD_SplashPushConstants
{
    float framebufferWidth;
    float framebufferHeight;
    float imageWidth;
    float imageHeight;
    float scale;
    float opacity;
} AVD_SplashPushConstants;

static bool __avdCreateDescriptorSetLayout(AVD_SplashScene* scene, VkDevice device)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);

    VkDescriptorSetLayoutBinding layoutBinding = {0};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &scene->descriptorSetLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create descriptor set layout\n");

    return true;
}

static bool __avdCreateDescriptorSet(AVD_SplashScene* scene, VkDevice device, VkDescriptorPool descriptorPool)
{
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(descriptorPool != VK_NULL_HANDLE);
    AVD_ASSERT(scene != NULL);

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &scene->descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &scene->descriptorSet);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate descriptor set\n");

    VkWriteDescriptorSet descriptorWrite = {0};
    AVD_CHECK(avdWriteImageDescriptorSet(&descriptorWrite, scene->descriptorSet, 0, &scene->splashImage.descriptorImageInfo));
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
    
    return true;
}

static bool __avdCreatePipelineLayout(AVD_SplashScene* scene, VkDevice device)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(AVD_SplashPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &scene->descriptorSetLayout;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    pipelineLayoutInfo.pushConstantRangeCount = AVD_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &scene->pipelineLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create pipeline layout\n");
    return true;
}

static bool __avdCreatePipeline(AVD_SplashScene* scene, VkDevice device, VkRenderPass renderPass)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = avdShaderModuleCreateFromAsset(device, "SplashVert");
    AVD_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");
    VkShaderModule fragmentShaderModule = avdShaderModuleCreateFromAsset(device, "SplashFrag");
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
    pipelineInfo.layout = scene->pipelineLayout;
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

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &scene->pipeline);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics pipeline\n");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool avdScenesSplashInit(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);
    AVD_SplashScene *scene = &gameState->scene.splashScene;
    AVD_CHECK(avdVulkanImageLoadFromAsset(&gameState->vulkan, "Splash", &scene->splashImage));
    AVD_CHECK(__avdCreateDescriptorSetLayout(scene, gameState->vulkan.device));
    AVD_CHECK(__avdCreateDescriptorSet(scene, gameState->vulkan.device, gameState->vulkan.descriptorPool));
    AVD_CHECK(__avdCreatePipelineLayout(scene, gameState->vulkan.device));
    AVD_CHECK(__avdCreatePipeline(scene, gameState->vulkan.device, gameState->vulkan.renderer.sceneFramebuffer.renderPass));
    return true;
}

void avdScenesSplashShutdown(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);
    AVD_SplashScene *scene = &gameState->scene.splashScene;

    avdVulkanImageDestroy(&gameState->vulkan, &scene->splashImage);
    vkDestroyDescriptorSetLayout(gameState->vulkan.device, scene->descriptorSetLayout, NULL);
    vkDestroyPipeline(gameState->vulkan.device, scene->pipeline, NULL);
    vkDestroyPipelineLayout(gameState->vulkan.device, scene->pipelineLayout, NULL);
}

bool avdScenesSplashSwitch(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);

    gameState->scene.currentScene = AVD_SCENE_TYPE_SPLASH;
    gameState->scene.splashScene.sceneStartTime = gameState->framerate.currentTime;
    gameState->scene.splashScene.sceneDurationLeft = 25.0;

    return true;
}

bool avdScenesSplashRender(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);

    uint32_t currentFrameIndex = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer commandBuffer = gameState->vulkan.renderer.resources[currentFrameIndex].commandBuffer;

    AVD_CHECK(avdBeginSceneRenderPass(
        commandBuffer,
        &gameState->vulkan.renderer,
        NULL, 0
    ));
    
    AVD_SplashPushConstants pushConstantData = {
        .framebufferWidth = (float)gameState->vulkan.renderer.sceneFramebuffer.width,
        .framebufferHeight = (float)gameState->vulkan.renderer.sceneFramebuffer.height,
        .imageWidth = (float)gameState->scene.splashScene.splashImage.width,
        .imageHeight = (float)gameState->scene.splashScene.splashImage.height,
        .scale = gameState->scene.splashScene.currentScale,  
        .opacity = gameState->scene.splashScene.currentOpacity, 
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameState->scene.splashScene.pipeline);
    vkCmdPushConstants(
        commandBuffer,
        gameState->scene.splashScene.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(AVD_SplashPushConstants),
        &pushConstantData
    );

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            gameState->scene.splashScene.pipelineLayout, 0, 1,
                            &gameState->scene.splashScene.descriptorSet, 0, NULL);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}

bool avdScenesSplashUpdate(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);

    float delayTime = 1.0f;
    float animationDuration = 3.0f;
    float totalSceneDuration = delayTime + animationDuration - 0.5f;

    float fadeInTime = 0.5f;
    float scaleBounceTime = 0.7f;
    float holdDuration = 1.5f;
    float fadeOutTime = 1.2f;

    float scaleUpEndTime = fadeInTime;
    float bounceEndTime = scaleUpEndTime + scaleBounceTime;
    float holdEndTime = bounceEndTime + holdDuration;
    float fadeOutStartTime = holdEndTime;

    float initialScale = 0.0f;
    float targetScale = 1.0f;
    float overshootScale = targetScale * 1.5f;

    double now = gameState->framerate.currentTime;
    double start = gameState->scene.splashScene.sceneStartTime;
    float t_total = (float)(now - start);

    float opacity = 0.0f;
    float scale = initialScale;

    if (t_total < delayTime) {
        opacity = 0.1f;
        scale = initialScale;
    } else {
        float t_anim = t_total - delayTime;

        if (t_anim < fadeInTime) {
            float nt = t_anim / fadeInTime;
            opacity = nt * nt;
            float scale_nt = 1.0f - powf(1.0f - nt, 3.0f);
            scale = initialScale + (overshootScale - initialScale) * scale_nt;

        } else if (t_anim < bounceEndTime) {
            opacity = 1.0f;
            float nt = (t_anim - scaleUpEndTime) / scaleBounceTime;

            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;
            float nt_inv = 1.0f - nt;
            float factor = 1.0f - (c3 * nt_inv * nt_inv * nt_inv - c1 * nt_inv * nt_inv);
            scale = targetScale + (overshootScale - targetScale) * (1.0f - factor);

        } else if (t_anim < holdEndTime) {
            opacity = 1.0f;
            scale = targetScale;
        } else if (t_anim < animationDuration) {
            float nt = (t_anim - fadeOutStartTime) / fadeOutTime;
            opacity = 1.0f - (nt * nt);
            scale = targetScale;
        } else {
            opacity = 0.0f;
            scale = targetScale;
            t_anim = animationDuration;
        }
        opacity = AVD_CLAMP(opacity, 0.0f, 1.0f);
        scale = AVD_MAX(scale, 0.0f);
    }


    gameState->scene.splashScene.currentScale = scale;
    gameState->scene.splashScene.currentOpacity = opacity;

	if (t_total >= totalSceneDuration && !gameState->scene.isSwitchingScene) {
        avdScenesSwitch(gameState, AVD_SCENE_TYPE_LOADING);
    }

    return true;
}
