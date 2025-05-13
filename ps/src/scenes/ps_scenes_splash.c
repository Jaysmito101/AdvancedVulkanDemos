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

static bool __psCreateDescriptorSetLayout(PS_SplashScene* scene, VkDevice device)
{
    PS_ASSERT(scene != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);

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
    PS_CHECK_VK_RESULT(result, "Failed to create descriptor set layout\n");

    return true;
}

static bool __psCreateDescriptorSet(PS_SplashScene* scene, VkDevice device, VkDescriptorPool descriptorPool)
{
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(descriptorPool != VK_NULL_HANDLE);
    PS_ASSERT(scene != NULL);

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &scene->descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &scene->descriptorSet);
    PS_CHECK_VK_RESULT(result, "Failed to allocate descriptor set\n");

    VkWriteDescriptorSet descriptorWrite = {0};
    PS_CHECK(psWriteImageDescriptorSet(&descriptorWrite, scene->descriptorSet, 0, &scene->splashImage.descriptorImageInfo));
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
    
    return true;
}

static bool __psCreatePipelineLayout(PS_SplashScene* scene, VkDevice device)
{
    PS_ASSERT(scene != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(PS_SplashPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &scene->descriptorSetLayout;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    pipelineLayoutInfo.pushConstantRangeCount = PS_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &scene->pipelineLayout);
    PS_CHECK_VK_RESULT(result, "Failed to create pipeline layout\n");
    return true;
}

static bool __psCreatePipeline(PS_SplashScene* scene, VkDevice device, VkRenderPass renderPass)
{
    PS_ASSERT(scene != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = psShaderModuleCreateFromAsset(device, "SplashVert");
    PS_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");
    VkShaderModule fragmentShaderModule = psShaderModuleCreateFromAsset(device, "SplashFrag");
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
    PS_CHECK_VK_RESULT(result, "Failed to create graphics pipeline\n");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool psScenesSplashInit(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    PS_SplashScene *scene = &gameState->scene.splashScene;
    PS_CHECK(psVulkanImageLoadFromAsset(&gameState->vulkan, "Splash", &scene->splashImage));
    PS_CHECK(__psCreateDescriptorSetLayout(scene, gameState->vulkan.device));
    PS_CHECK(__psCreateDescriptorSet(scene, gameState->vulkan.device, gameState->vulkan.descriptorPool));
    PS_CHECK(__psCreatePipelineLayout(scene, gameState->vulkan.device));
    PS_CHECK(__psCreatePipeline(scene, gameState->vulkan.device, gameState->vulkan.renderer.sceneFramebuffer.renderPass));
    return true;
}

void psScenesSplashShutdown(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    PS_SplashScene *scene = &gameState->scene.splashScene;

    psVulkanImageDestroy(&gameState->vulkan, &scene->splashImage);
    vkDestroyDescriptorSetLayout(gameState->vulkan.device, scene->descriptorSetLayout, NULL);
    vkDestroyPipeline(gameState->vulkan.device, scene->pipeline, NULL);
    vkDestroyPipelineLayout(gameState->vulkan.device, scene->pipelineLayout, NULL);
}

bool psScenesSplashSwitch(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    gameState->scene.currentScene = PS_SCENE_TYPE_SPLASH;
    gameState->scene.splashScene.sceneStartTime = gameState->framerate.currentTime;
    gameState->scene.splashScene.sceneDurationLeft = 25.0;

    return true;
}

bool psScenesSplashRender(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    uint32_t currentFrameIndex = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer commandBuffer = gameState->vulkan.renderer.resources[currentFrameIndex].commandBuffer;

    PS_CHECK(psBeginSceneRenderPass(
        commandBuffer,
        &gameState->vulkan.renderer,
        NULL, 0
    ));
    
    PS_SplashPushConstants pushConstantData = {
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
        0, sizeof(PS_SplashPushConstants),
        &pushConstantData
    );

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            gameState->scene.splashScene.pipelineLayout, 0, 1,
                            &gameState->scene.splashScene.descriptorSet, 0, NULL);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    PS_CHECK(psEndSceneRenderPass(commandBuffer));

    return true;
}

bool psScenesSplashUpdate(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

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
        opacity = PS_CLAMP(opacity, 0.0f, 1.0f);
        scale = PS_MAX(scale, 0.0f);
    }


    gameState->scene.splashScene.currentScale = scale;
    gameState->scene.splashScene.currentOpacity = opacity;

	if (t_total >= totalSceneDuration && !gameState->scene.isSwitchingScene) {
        psScenesSwitch(gameState, PS_SCENE_TYPE_LOADING);
    }

    return true;
}
