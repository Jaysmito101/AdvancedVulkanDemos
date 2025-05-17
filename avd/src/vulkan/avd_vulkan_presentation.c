#include "vulkan/avd_vulkan.h"
#include "shader/avd_shader.h"
#include "scenes/avd_scenes.h"

// Define the push constant struct matching the shader layout
typedef struct AVD_VulkanPresentationPushConstants
{
    float windowWidth;
    float windowHeight;
    float framebufferWidth;
    float framebufferHeight;
    
    float sceneLoadingProgress;
    float time;
    float pad0;
    float pad1;
} AVD_VulkanPresentationPushConstants;

static bool __avdVulkanPresentationCreatePipeline(AVD_VulkanPresentation *presentation, VkDevice device, VkRenderPass renderPass)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = avdShaderModuleCreateFromAsset(device, "PresentationVert");
    AVD_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");

    VkShaderModule fragmentShaderModule = avdShaderModuleCreateFromAsset(device, "PresentationFrag");
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
    pipelineInfo.layout = presentation->pipelineLayout;
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

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &presentation->pipeline);
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics pipeline for presentation");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool avdVulkanPresentationInit(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain, AVD_FontRenderer *fontRenderer)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(vulkan != NULL);
    
    AVD_CHECK(avdCreateDescriptorSetLayout(
        &presentation->descriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));

    AVD_CHECK(avdPipelineUtilsCreateGraphicsPipelineLayout(
        &presentation->pipelineLayout,
        vulkan->device,
        &presentation->descriptorSetLayout, 1,
        sizeof(AVD_VulkanPresentationPushConstants)));
    AVD_CHECK(__avdVulkanPresentationCreatePipeline(presentation, vulkan->device, swapchain->renderPass));
    AVD_CHECK(avdRenderableTextCreate(
        &presentation->loadingText,
        fontRenderer,
        vulkan,
        "OpenSansRegular",
        "Loading...",
        24.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &presentation->loadingStatusText,
        fontRenderer,
        vulkan,
        "OpenSansRegular",
        "No status",
        16.0f));
    return true;
}

void avdVulkanPresentationDestroy(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(vulkan != NULL);

    avdRenderableTextDestroy(&presentation->loadingStatusText, vulkan);
    avdRenderableTextDestroy(&presentation->loadingText, vulkan);
    vkDestroyPipeline(vulkan->device, presentation->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, presentation->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, presentation->descriptorSetLayout, NULL);
}

bool avdVulkanPresentationRender(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanRenderer *renderer, AVD_VulkanSwapchain *swapchain, AVD_SceneManager *sceneManager, AVD_FontRenderer *fontRenderer, uint32_t imageIndex)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(swapchain != NULL);

    uint32_t currentFrameIndex = renderer->currentFrameIndex;
    VkCommandBuffer commandBuffer = renderer->resources[currentFrameIndex].commandBuffer;

    static VkClearValue defaultClearValues[2] = {
        {.color = {.float32 = {0.0f, 0.0f, 0.0f}}},
        {.depthStencil = {.depth = 1.0f, .stencil = 0}}};

    AVD_CHECK(avdBeginRenderPass(
        commandBuffer,
        swapchain->renderPass,
        swapchain->framebuffers[imageIndex],
        swapchain->extent.width,
        swapchain->extent.height,
        defaultClearValues,
        AVD_ARRAY_COUNT(defaultClearValues)));

    // Populate the push constant struct
    AVD_VulkanPresentationPushConstants pushConstants = {
        .windowWidth = (float)swapchain->extent.width,
        .windowHeight = (float)swapchain->extent.height,
        .framebufferWidth = (float)renderer->sceneFramebuffer.width,
        .framebufferHeight = (float)renderer->sceneFramebuffer.height,
        .sceneLoadingProgress = sceneManager->sceneLoadingProgress,
        .time = (float)glfwGetTime(),
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentation->pipeline);
    vkCmdPushConstants(commandBuffer, presentation->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AVD_VulkanPresentationPushConstants), &pushConstants);
    VkDescriptorSet descriptorSetsToBind[] = {
        renderer->sceneFramebuffer.colorAttachment.descriptorSet, // Set 0
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentation->pipelineLayout, 0, AVD_ARRAY_COUNT(descriptorSetsToBind), descriptorSetsToBind, 0, NULL);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    // NOTE: This wont work for now, as the font renderer pipeline only works with
    //       the scene framebuffer and not the swapchain framebuffer, this will be fixed later?
#if 0
    if (!sceneManager->isSceneLoaded)
    {
        static char loadingText[256];
        snprintf(loadingText, sizeof(loadingText), "Loading... [%.2f%%]", sceneManager->sceneLoadingProgress * 100.0f);
        AVD_CHECK(avdRenderableTextUpdate(
            &presentation->loadingText,
            fontRenderer,
            vulkan,
            loadingText));

        avdRenderText(
            vulkan,
            fontRenderer,
            &presentation->loadingText,
            commandBuffer,
            100.0f, 100.0f,
            1.0f,
            1.0f, 1.0f, 0.0f, 1.0f,
            swapchain->extent.width, swapchain->extent.height);

        if (sceneManager->sceneLoadingStatusMessage != NULL)
        {
            AVD_CHECK(avdRenderableTextUpdate(
                &presentation->loadingStatusText,
                fontRenderer,
                vulkan,
                sceneManager->sceneLoadingStatusMessage));

            avdRenderText(
                vulkan,
                fontRenderer,
                &presentation->loadingStatusText,
                commandBuffer,
                100.0f, 500.0f,
                1.0f,
                1.0f, 1.0f, 0.0f, 1.0f,
                swapchain->extent.width, swapchain->extent.height);
        }
    }
#endif

    AVD_CHECK(avdEndRenderPass(commandBuffer));
    return true;
}