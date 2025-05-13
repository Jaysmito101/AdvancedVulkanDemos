#include "ps_vulkan.h"
#include "ps_scenes.h"
#include "ps_shader.h"

// Define the push constant struct matching the shader layout
typedef struct PS_VulkanPresentationPushConstants {
    float windowWidth;
    float windowHeight;
    float framebufferWidth;
    float framebufferHeight;
    float circleRadius;
    float iconWidth;
    float iconHeight;
    float time;
} PS_VulkanPresentationPushConstants;


static bool __psVulkanPresentationCreatePipelineLayout(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);
    // Ensure descriptor set layouts are created before this function is called
    PS_ASSERT(vulkan->renderer.presentation.descriptorSetLayout != VK_NULL_HANDLE);
    PS_ASSERT(vulkan->renderer.presentation.iconDescriptorSetLayout != VK_NULL_HANDLE);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(PS_VulkanPresentationPushConstants); // Use sizeof the struct

    // Combine layouts for the pipeline layout
    VkDescriptorSetLayout setLayouts[] = {
        vulkan->renderer.presentation.descriptorSetLayout,
        vulkan->renderer.presentation.iconDescriptorSetLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = PS_ARRAY_COUNT(setLayouts);
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    pipelineLayoutInfo.pushConstantRangeCount = PS_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(vulkan->device, &pipelineLayoutInfo, NULL, &vulkan->renderer.presentation.pipelineLayout);
    PS_CHECK_VK_RESULT(result, "Failed to create pipeline layout for presentation");

    return true;
}

static bool __psVulkanPresentationCreatePipeline(PS_VulkanPresentation *presentation, VkDevice device, VkRenderPass renderPass) {
    PS_ASSERT(presentation != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = psShaderModuleCreateFromAsset(device, "PresentationVert");
    PS_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");

    VkShaderModule fragmentShaderModule = psShaderModuleCreateFromAsset(device, "PresentationFrag");
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
    PS_CHECK_VK_RESULT(result, "Failed to create graphics pipeline for presentation");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

static bool __psVulkanPresentationSetupDescriptors(PS_Vulkan* vulkan) {
    PS_ASSERT(vulkan != NULL);

    PS_VulkanPresentation *presentation = &vulkan->renderer.presentation;

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
    PS_CHECK_VK_RESULT(sceneLayoutResult, "Failed to create scene framebuffer descriptor set layout");

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
    PS_CHECK_VK_RESULT(iconLayoutResult, "Failed to create icon descriptor set layout");

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vulkan->descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &presentation->iconDescriptorSetLayout;

    VkResult allocResult = vkAllocateDescriptorSets(vulkan->device, &allocInfo, &presentation->iconDescriptorSet);
    PS_CHECK_VK_RESULT(allocResult, "Failed to allocate icon descriptor set");

    // Update the icon descriptor set
    VkWriteDescriptorSet descriptorWrite = {0};
    PS_CHECK(psWriteImageDescriptorSet(&descriptorWrite, presentation->iconDescriptorSet, 0, &presentation->iconImage.descriptorImageInfo));

    vkUpdateDescriptorSets(vulkan->device, 1, &descriptorWrite, 0, NULL);

    return true;
}

bool psVulkanPresentationInit(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);
    PS_VulkanPresentation *presentation = &vulkan->renderer.presentation;
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "SwitchMascot", &presentation->iconImage));
    PS_CHECK(__psVulkanPresentationSetupDescriptors(vulkan));
    PS_CHECK(__psVulkanPresentationCreatePipelineLayout(vulkan));
    PS_CHECK(__psVulkanPresentationCreatePipeline(presentation, vulkan->device, vulkan->swapchain.renderPass));
    presentation->circleRadius = 1.5f;
    return true;
}

void psVulkanPresentationDestroy(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);

    PS_VulkanPresentation *presentation = &vulkan->renderer.presentation;

    psVulkanImageDestroy(vulkan, &presentation->iconImage);
    // Note: Icon descriptor set is implicitly freed when the pool is destroyed.
    vkDestroyPipeline(vulkan->device, presentation->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, presentation->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, presentation->descriptorSetLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, presentation->iconDescriptorSetLayout, NULL);
}

bool psVulkanPresentationRender(PS_Vulkan *vulkan, uint32_t imageIndex)
{
    PS_ASSERT(vulkan != NULL);

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
    PS_VulkanPresentationPushConstants pushConstants = {
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
    vkCmdPushConstants(commandBuffer, vulkan->renderer.presentation.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PS_VulkanPresentationPushConstants), &pushConstants);

    VkDescriptorSet descriptorSetsToBind[] = {
        vulkan->renderer.sceneFramebuffer.colorAttachment.descriptorSet, // Set 0
        vulkan->renderer.presentation.iconDescriptorSet      // Set 1
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->renderer.presentation.pipelineLayout, 0, PS_ARRAY_COUNT(descriptorSetsToBind), descriptorSetsToBind, 0, NULL);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    return true;
}