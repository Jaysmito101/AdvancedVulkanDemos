#include "avd_vulkan.h"
#include "avd_scenes.h"
#include "avd_shader.h"

// Define the push constant struct matching the shader layout
typedef struct AVD_VulkanPresentationPushConstants {
    float windowWidth;
    float windowHeight;
    float framebufferWidth;
    float framebufferHeight;
    float circleRadius;
    float iconWidth;
    float iconHeight;
    float time;
} AVD_VulkanPresentationPushConstants;


static bool __avdVulkanPresentationCreatePipelineLayout(AVD_Vulkan *vulkan) {
    AVD_ASSERT(vulkan != NULL);
    // Ensure descriptor set layouts are created before this function is called
    AVD_ASSERT(vulkan->renderer.presentation.descriptorSetLayout != VK_NULL_HANDLE);
    AVD_ASSERT(vulkan->renderer.presentation.iconDescriptorSetLayout != VK_NULL_HANDLE);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(AVD_VulkanPresentationPushConstants); // Use sizeof the struct

    // Combine layouts for the pipeline layout
    VkDescriptorSetLayout setLayouts[] = {
        vulkan->renderer.presentation.descriptorSetLayout,
        vulkan->renderer.presentation.iconDescriptorSetLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = AVD_ARRAY_COUNT(setLayouts);
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    pipelineLayoutInfo.pushConstantRangeCount = AVD_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(vulkan->device, &pipelineLayoutInfo, NULL, &vulkan->renderer.presentation.pipelineLayout);
    AVD_CHECK_VK_RESULT(result, "Failed to create pipeline layout for presentation");

    return true;
}

static bool __avdVulkanPresentationCreatePipeline(AVD_VulkanPresentation *presentation, VkDevice device, VkRenderPass renderPass) {
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

static bool __avdVulkanPresentationSetupDescriptors(AVD_Vulkan* vulkan) {
    AVD_ASSERT(vulkan != NULL);

    AVD_VulkanPresentation *presentation = &vulkan->renderer.presentation;

    VkDescriptorSetLayoutBinding sceneFramebufferBinding = {0};
    sceneFramebufferBinding.binding = 0;
    sceneFramebufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sceneFramebufferBinding.descriptorCount = 1;
    sceneFramebufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo sceneFramebufferLayoutInfo = {0};
    sceneFramebufferLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sceneFramebufferLayoutInfo.bindingCount = 1;
    sceneFramebufferLayoutInfo.pBindings = &sceneFramebufferBinding;

    VkResult sceneLayoutResult = vkCreateDescriptorSetLayout(vulkan->device, &sceneFramebufferLayoutInfo, NULL, &presentation->descriptorSetLayout);
    AVD_CHECK_VK_RESULT(sceneLayoutResult, "Failed to create scene framebuffer descriptor set layout");

    VkDescriptorSetLayoutBinding iconBinding = {0};
    iconBinding.binding = 0; 
    iconBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    iconBinding.descriptorCount = 1;
    iconBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo iconLayoutInfo = {0};
    iconLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    iconLayoutInfo.bindingCount = 1;
    iconLayoutInfo.pBindings = &iconBinding;

    VkResult iconLayoutResult = vkCreateDescriptorSetLayout(vulkan->device, &iconLayoutInfo, NULL, &presentation->iconDescriptorSetLayout);
    AVD_CHECK_VK_RESULT(iconLayoutResult, "Failed to create icon descriptor set layout");

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vulkan->descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &presentation->iconDescriptorSetLayout;

    VkResult allocResult = vkAllocateDescriptorSets(vulkan->device, &allocInfo, &presentation->iconDescriptorSet);
    AVD_CHECK_VK_RESULT(allocResult, "Failed to allocate icon descriptor set");

    // Update the icon descriptor set
    VkWriteDescriptorSet descriptorWrite = {0};
    AVD_CHECK(avdWriteImageDescriptorSet(&descriptorWrite, presentation->iconDescriptorSet, 0, &presentation->iconImage.descriptorImageInfo));

    vkUpdateDescriptorSets(vulkan->device, 1, &descriptorWrite, 0, NULL);

    return true;
}

bool avdVulkanPresentationInit(AVD_Vulkan *vulkan) {
    AVD_ASSERT(vulkan != NULL);
    AVD_VulkanPresentation *presentation = &vulkan->renderer.presentation;
    AVD_CHECK(avdVulkanImageLoadFromAsset(vulkan, "SwitchMascot", &presentation->iconImage));
    AVD_CHECK(__avdVulkanPresentationSetupDescriptors(vulkan));
    AVD_CHECK(__avdVulkanPresentationCreatePipelineLayout(vulkan));
    AVD_CHECK(__avdVulkanPresentationCreatePipeline(presentation, vulkan->device, vulkan->swapchain.renderPass));
    presentation->circleRadius = 1.5f;
    return true;
}

void avdVulkanPresentationDestroy(AVD_Vulkan *vulkan) {
    AVD_ASSERT(vulkan != NULL);

    AVD_VulkanPresentation *presentation = &vulkan->renderer.presentation;

    avdVulkanImageDestroy(vulkan, &presentation->iconImage);
    // Note: Icon descriptor set is implicitly freed when the pool is destroyed.
    vkDestroyPipeline(vulkan->device, presentation->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, presentation->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, presentation->descriptorSetLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, presentation->iconDescriptorSetLayout, NULL);
}

bool avdVulkanPresentationRender(AVD_Vulkan *vulkan, uint32_t imageIndex)
{
    AVD_ASSERT(vulkan != NULL);

    uint32_t currentFrameIndex = vulkan->renderer.currentFrameIndex;
    VkCommandBuffer commandBuffer = vulkan->renderer.resources[currentFrameIndex].commandBuffer;

    static VkClearValue clearColor[2] = {0};
    clearColor[0].color.float32[0] = 0.0f;
    clearColor[0].color.float32[1] = 0.0f;
    clearColor[0].color.float32[2] = 0.0f;
    clearColor[0].color.float32[3] = 1.0f;

    clearColor[1].depthStencil.depth = 1.0f;
    clearColor[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkan->swapchain.renderPass;
    renderPassInfo.framebuffer = vulkan->swapchain.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = vulkan->swapchain.extent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkan->swapchain.extent.width;
    viewport.height = (float)vulkan->swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = vulkan->swapchain.extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Populate the push constant struct
    AVD_VulkanPresentationPushConstants pushConstants = {
        .windowWidth = (float)vulkan->swapchain.extent.width,
        .windowHeight = (float)vulkan->swapchain.extent.height,
        .framebufferWidth = (float)vulkan->renderer.sceneFramebuffer.width,
        .framebufferHeight = (float)vulkan->renderer.sceneFramebuffer.height,
        .circleRadius = vulkan->renderer.presentation.circleRadius,
        .iconWidth = (float)vulkan->renderer.presentation.iconImage.width,
        .iconHeight = (float)vulkan->renderer.presentation.iconImage.height,
        .time = (float)glfwGetTime(),
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->renderer.presentation.pipeline);
    vkCmdPushConstants(commandBuffer, vulkan->renderer.presentation.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AVD_VulkanPresentationPushConstants), &pushConstants);

    VkDescriptorSet descriptorSetsToBind[] = {
        vulkan->renderer.sceneFramebuffer.colorAttachment.descriptorSet, // Set 0
        vulkan->renderer.presentation.iconDescriptorSet      // Set 1
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->renderer.presentation.pipelineLayout, 0, AVD_ARRAY_COUNT(descriptorSetsToBind), descriptorSetsToBind, 0, NULL);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    return true;
}