#include "ps_scenes.h"
#include "ps_shader.h"

// Push constant struct matching shader data
typedef struct PS_MainMenuPushConstants {
    float windowWidth;      
    float windowHeight;     
    float time;
    float backgroundImageWidth;
    float backgroundImageHeight;
    float buttonImageWidth;
    float buttonImageHeight;
    float hoverScaleFactor;
    float hoverOffsetY;
    int32_t hoveredButton; // 0: None, 1: New Game, 2: Continue, 3: Options, 4: Exit
    int32_t continueDisabled; // 0: Enabled, 1: Disabled
} PS_MainMenuPushConstants;

bool psScenesMainMenuInit(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;
 
    // --- Load Textures ---
    size_t imageDataSize = 0;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("TitleScreen", &imageDataSize), imageDataSize, &scene->backgroundTexture)) return false;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("TitleScreenNewGame", &imageDataSize), imageDataSize, &scene->newGameButtonTexture)) return false;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("TitleScreenContinue", &imageDataSize), imageDataSize, &scene->continueButtonTexture)) return false;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("TitleScreenOptions", &imageDataSize), imageDataSize, &scene->optionsButtonTexture)) return false;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("TitleScreenExit", &imageDataSize), imageDataSize, &scene->exitButtonTexture)) return false;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("MascotHope", &imageDataSize), imageDataSize, &scene->mascotHopeTexture)) return false;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("MascotCrush", &imageDataSize), imageDataSize, &scene->mascotCrushTexture)) return false;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("MascotMonster", &imageDataSize), imageDataSize, &scene->mascotMonsterTexture)) return false;
    if (!psVulkanImageLoadFromMemory(gameState, psAssetImage("MascotFriend", &imageDataSize), imageDataSize, &scene->mascotFriendTexture)) return false;

    // --- Create Shader Modules ---
    VkShaderModule vert = psShaderModuleCreate(gameState, psShader_MainMenuVertex, VK_SHADER_STAGE_VERTEX_BIT, "main_menu_vertex_shader.glsl");
    if (vert == VK_NULL_HANDLE) {
        PS_LOG("Failed to create main menu vertex shader module\n");
        return false;
    }
    scene->vertexShaderModule = vert;

    VkShaderModule frag = psShaderModuleCreate(gameState, psShader_MainMenuFragment, VK_SHADER_STAGE_FRAGMENT_BIT, "main_menu_fragment_shader.glsl");
    if (frag == VK_NULL_HANDLE) {
        PS_LOG("Failed to create main menu fragment shader module\n");
        return false;
    }
    scene->fragmentShaderModule = frag;

    VkDescriptorSetLayoutBinding bindings[9] = {0};
    // Background Texture
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Button Textures
    bindings[1].binding = 1; // New Game
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[2].binding = 2; // Continue
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[3].binding = 3; // Options
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[4].binding = 4; // Exit
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Mascot Textures
    bindings[5].binding = 5; // Hope
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[6].binding = 6; // Crush
    bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[6].descriptorCount = 1;
    bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[7].binding = 7; // Monster
    bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[7].descriptorCount = 1;
    bindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[8].binding = 8; // Friend
    bindings[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[8].descriptorCount = 1;
    bindings[8].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {0};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = PS_ARRAY_COUNT(bindings);
    setLayoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(gameState->vulkan.device, &setLayoutInfo, NULL, &scene->textureDescriptorSetLayout) != VK_SUCCESS) {
        PS_LOG("Failed to create main menu texture descriptor set layout\n");
        return false;
    }

    // --- Pipeline Layout ---
    VkPushConstantRange pushRange = {0};
    pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Only fragment shader uses push constants now
    pushRange.offset = 0;
    pushRange.size = sizeof(PS_MainMenuPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1; // Use the texture descriptor set layout
    layoutInfo.pSetLayouts = &scene->textureDescriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    if (vkCreatePipelineLayout(gameState->vulkan.device, &layoutInfo, NULL, &scene->pipelineLayout) != VK_SUCCESS) {
        PS_LOG("Failed to create main menu pipeline layout\n");
        return false;
    }

    // --- Allocate Descriptor Set ---
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = gameState->vulkan.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &scene->textureDescriptorSetLayout;

    if (vkAllocateDescriptorSets(gameState->vulkan.device, &allocInfo, &scene->textureDescriptorSet) != VK_SUCCESS) {
        PS_LOG("Failed to allocate main menu texture descriptor set\n");
        return false;
    }

    // --- Update Descriptor Set ---
    VkWriteDescriptorSet writes[9] = {0};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = scene->textureDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = &scene->backgroundTexture.descriptorImageInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = scene->textureDescriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &scene->newGameButtonTexture.descriptorImageInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = scene->textureDescriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].pImageInfo = &scene->continueButtonTexture.descriptorImageInfo;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = scene->textureDescriptorSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].pImageInfo = &scene->optionsButtonTexture.descriptorImageInfo;

    writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[4].dstSet = scene->textureDescriptorSet;
    writes[4].dstBinding = 4;
    writes[4].descriptorCount = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[4].pImageInfo = &scene->exitButtonTexture.descriptorImageInfo;

    writes[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[5].dstSet = scene->textureDescriptorSet;
    writes[5].dstBinding = 5;
    writes[5].descriptorCount = 1;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[5].pImageInfo = &scene->mascotHopeTexture.descriptorImageInfo;

    writes[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[6].dstSet = scene->textureDescriptorSet;
    writes[6].dstBinding = 6;
    writes[6].descriptorCount = 1;
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[6].pImageInfo = &scene->mascotCrushTexture.descriptorImageInfo;

    writes[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[7].dstSet = scene->textureDescriptorSet;
    writes[7].dstBinding = 7;
    writes[7].descriptorCount = 1;
    writes[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[7].pImageInfo = &scene->mascotMonsterTexture.descriptorImageInfo;

    writes[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[8].dstSet = scene->textureDescriptorSet;
    writes[8].dstBinding = 8;
    writes[8].descriptorCount = 1;
    writes[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[8].pImageInfo = &scene->mascotFriendTexture.descriptorImageInfo;

    vkUpdateDescriptorSets(gameState->vulkan.device, PS_ARRAY_COUNT(writes), writes, 0, NULL);

    // --- Graphics pipeline ---
    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vert; // Use the temporary handle
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = frag; // Use the temporary handle
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
    pipelineInfo.layout = scene->pipelineLayout; // Use the updated layout
    pipelineInfo.renderPass = gameState->vulkan.renderer.sceneFramebuffer.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(gameState->vulkan.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &scene->pipeline) != VK_SUCCESS) {
        PS_LOG("Failed to create main menu graphics pipeline\n");
        return false;
    }

    vkDestroyShaderModule(gameState->vulkan.device, scene->vertexShaderModule, NULL);
    vkDestroyShaderModule(gameState->vulkan.device, scene->fragmentShaderModule, NULL);
    scene->vertexShaderModule = VK_NULL_HANDLE;
    scene->fragmentShaderModule = VK_NULL_HANDLE;
    scene->wasMouseClicked = false;
    scene->continueDisabled = true; // Disable continue button by default 

    return true;
}

void psScenesMainMenuShutdown(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;

    // Destroy pipeline and layout first
    vkDestroyPipeline(gameState->vulkan.device, scene->pipeline, NULL);
    vkDestroyPipelineLayout(gameState->vulkan.device, scene->pipelineLayout, NULL);

    // Cleanup shader modules now they are linked to the pipeline
    vkDestroyShaderModule(gameState->vulkan.device, scene->vertexShaderModule, NULL);
    vkDestroyShaderModule(gameState->vulkan.device, scene->fragmentShaderModule, NULL);

    // Destroy descriptor set layout (descriptor set is implicitly freed with pool)
    vkDestroyDescriptorSetLayout(gameState->vulkan.device, scene->textureDescriptorSetLayout, NULL);

    // Destroy textures
    psVulkanImageDestroy(gameState, &scene->backgroundTexture);
    psVulkanImageDestroy(gameState, &scene->newGameButtonTexture);
    psVulkanImageDestroy(gameState, &scene->continueButtonTexture);
    psVulkanImageDestroy(gameState, &scene->optionsButtonTexture);
    psVulkanImageDestroy(gameState, &scene->exitButtonTexture);
    psVulkanImageDestroy(gameState, &scene->mascotHopeTexture);
    psVulkanImageDestroy(gameState, &scene->mascotCrushTexture);
    psVulkanImageDestroy(gameState, &scene->mascotMonsterTexture);
    psVulkanImageDestroy(gameState, &scene->mascotFriendTexture);
}

bool psScenesMainMenuSwitch(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    return true;
}

bool psScenesMainMenuUpdate(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;

    float mx = gameState->input.mouseX * 0.5f + 0.5f;
    float my = gameState->input.mouseY * 0.5f + 0.5f;

    // Button layout constants
    const float buttonHeightUV     = 0.1f;
    const float buttonCenterX      = 0.5f;
    const float buttonCenterYStart = 0.45f;
    const float buttonSpacingY     = 0.12f;

    // Compute button width in UV (accounting for window aspect & image aspect)
    float buttonImageAspect = (float)scene->newGameButtonTexture.width / scene->newGameButtonTexture.height;
    float windowAspect = (float)gameState->vulkan.renderer.sceneFramebuffer.width / (float)gameState->vulkan.renderer.sceneFramebuffer.height;
    float buttonWidthUV = buttonImageAspect * buttonHeightUV / windowAspect;
    
    
    
    for (int i = 0; i < PS_MAIN_MENU_BUTTON_COUNT; ++i) {
        float buttonCenterY = buttonCenterYStart - (buttonSpacingY * i);
        float buttonXMin = buttonCenterX - (buttonWidthUV / 2.0f);
        float buttonXMax = buttonCenterX + (buttonWidthUV / 2.0f);
        float buttonYMin = buttonCenterY - (buttonHeightUV / 2.0f);
        float buttonYMax = buttonCenterY + (buttonHeightUV / 2.0f);
        
        if (mx >= buttonXMin && mx <= buttonXMax && my >= buttonYMin && my <= buttonYMax) {
            scene->hoveredButton = i + 1; 
            break;
        } else {
            scene->hoveredButton = PS_MAIN_MENU_BUTTON_NONE;
        }
    }

    // Check if the continue button is disabled then set hovered button to none
    if (scene->hoveredButton == PS_MAIN_MENU_BUTTON_CONTINUE && scene->continueDisabled) {
        scene->hoveredButton = PS_MAIN_MENU_BUTTON_NONE;
    }

    bool isMouseClicked = gameState->input.mouseButtonState[GLFW_MOUSE_BUTTON_LEFT];
    if (isMouseClicked && !scene->wasMouseClicked) {
        switch (scene->hoveredButton) {
            case PS_MAIN_MENU_BUTTON_NEW_GAME:
                psScenesSwitch(gameState, PS_SCENE_TYPE_PROLOGUE);
                break;
            case PS_MAIN_MENU_BUTTON_CONTINUE:
                PS_LOG("Continue button clicked [NOT IMPLEMENTED YET!]\n");
                break;
            case PS_MAIN_MENU_BUTTON_OPTIONS:
                PS_LOG("Options button clicked [NOT IMPLEMENTED YET!]\n");
                psScenesSwitch(gameState, PS_SCENE_TYPE_SPLASH);
                break;
            case PS_MAIN_MENU_BUTTON_EXIT:
                PS_LOG("Exit button clicked\n");
                gameState->running = false;
                break;
            default:
                break;
        }
    }
    scene->wasMouseClicked = isMouseClicked;
    
    return true;
}

bool psScenesMainMenuRender(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;
    uint32_t idx = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer cmd = gameState->vulkan.renderer.resources[idx].commandBuffer;

    // Begin scene render pass
    VkRenderPassBeginInfo rpInfo = {0};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = gameState->vulkan.renderer.sceneFramebuffer.renderPass;
    rpInfo.framebuffer = gameState->vulkan.renderer.sceneFramebuffer.framebuffer;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent.width = gameState->vulkan.renderer.sceneFramebuffer.width;
    rpInfo.renderArea.extent.height = gameState->vulkan.renderer.sceneFramebuffer.height;
    VkClearValue clears[2] = {0};
    
    clears[0].color.float32[0] = 208.0f / 255.0f;
    clears[0].color.float32[1] = 166.0f / 255.0f;
    clears[0].color.float32[2] = 228.0f / 255.0f;
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

    // Bind Descriptor Set
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->pipelineLayout,
                            0, // firstSet
                            1, // setCount
                            &scene->textureDescriptorSet,
                            0, NULL); // dynamicOffsets

    bool isMouseClicked = gameState->input.mouseButtonState[GLFW_MOUSE_BUTTON_LEFT];
    PS_MainMenuPushConstants pc = {
        .windowWidth = (float)gameState->vulkan.renderer.sceneFramebuffer.width,
        .windowHeight = (float)gameState->vulkan.renderer.sceneFramebuffer.height,
        .time = (float)gameState->framerate.currentTime,
        .backgroundImageWidth = (float)scene->backgroundTexture.width,
        .backgroundImageHeight = (float)scene->backgroundTexture.height,
        .buttonImageWidth = (float)scene->newGameButtonTexture.width,
        .buttonImageHeight = (float)scene->newGameButtonTexture.height,
        .hoverScaleFactor = !isMouseClicked ? 1.05f : 0.95f,
        .hoverOffsetY = isMouseClicked ? 0.005f : -0.005f,
        .hoveredButton = scene->hoveredButton,
        .continueDisabled = scene->continueDisabled ? 1 : 0,
    };
    vkCmdPushConstants(cmd, scene->pipelineLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pc), &pc);

    // Draw full screen quad (6 vertices)
    vkCmdDraw(cmd, 6, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
    return true;
}
