#include "ps_scenes.h"
#include "ps_shader.h"

// Push constant struct matching shader data
typedef struct PS_MainMenuPushConstants
{
    float windowWidth;
    float windowHeight;
    float time;
    float backgroundImageWidth;
    float backgroundImageHeight;
    float buttonImageWidth;
    float buttonImageHeight;
    float hoverScaleFactor;
    float hoverOffsetY;
    int32_t hoveredButton;    // 0: None, 1: New Game, 2: Continue, 3: Options, 4: Exit
    int32_t continueDisabled; // 0: Enabled, 1: Disabled
} PS_MainMenuPushConstants;

static bool __psCreateDescriptorSetLayout(PS_MainMenuScene *scene, VkDevice device)
{
    PS_ASSERT(scene != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);

    VkDescriptorSetLayoutBinding layoutBindings[9] = {0};

    // Background Texture
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Button Textures
    for (int i = 1; i < 5; ++i)
    {
        layoutBindings[i].binding = i;
        layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[i].descriptorCount = 1;
        layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    // Mascot Textures
    for (int i = 5; i < 9; ++i)
    {
        layoutBindings[i].binding = i;
        layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[i].descriptorCount = 1;
        layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {0};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = PS_ARRAY_COUNT(layoutBindings);
    setLayoutInfo.pBindings = layoutBindings;

    VkResult result = vkCreateDescriptorSetLayout(device, &setLayoutInfo, NULL, &scene->textureDescriptorSetLayout);
    PS_CHECK_VK_RESULT(result, "Failed to create main menu texture descriptor set layout\n");

    return true;
}

static bool __psCreateDescriptorSet(PS_MainMenuScene *scene, VkDevice device, VkDescriptorPool descriptorPool)
{
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(descriptorPool != VK_NULL_HANDLE);
    PS_ASSERT(scene != NULL);

    // --- Allocate Descriptor Set ---
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &scene->textureDescriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &scene->textureDescriptorSet);
    PS_CHECK_VK_RESULT(result, "Failed to allocate main menu texture descriptor set\n");

    // --- Update Descriptor Set ---
    VkWriteDescriptorSet writes[9] = {0};
    PS_CHECK(psWriteImageDescriptorSet(&writes[0], scene->textureDescriptorSet, 0, &scene->backgroundTexture.descriptorImageInfo));
    PS_CHECK(psWriteImageDescriptorSet(&writes[1], scene->textureDescriptorSet, 1, &scene->newGameButtonTexture.descriptorImageInfo));
    PS_CHECK(psWriteImageDescriptorSet(&writes[2], scene->textureDescriptorSet, 2, &scene->continueButtonTexture.descriptorImageInfo));
    PS_CHECK(psWriteImageDescriptorSet(&writes[3], scene->textureDescriptorSet, 3, &scene->optionsButtonTexture.descriptorImageInfo));
    PS_CHECK(psWriteImageDescriptorSet(&writes[4], scene->textureDescriptorSet, 4, &scene->exitButtonTexture.descriptorImageInfo));
    PS_CHECK(psWriteImageDescriptorSet(&writes[5], scene->textureDescriptorSet, 5, &scene->mascotHopeTexture.descriptorImageInfo));
    PS_CHECK(psWriteImageDescriptorSet(&writes[6], scene->textureDescriptorSet, 6, &scene->mascotCrushTexture.descriptorImageInfo));
    PS_CHECK(psWriteImageDescriptorSet(&writes[7], scene->textureDescriptorSet, 7, &scene->mascotMonsterTexture.descriptorImageInfo));
    PS_CHECK(psWriteImageDescriptorSet(&writes[8], scene->textureDescriptorSet, 8, &scene->mascotFriendTexture.descriptorImageInfo));
    vkUpdateDescriptorSets(device, PS_ARRAY_COUNT(writes), writes, 0, NULL);
    return true;
}

static bool __psCreatePipelineLayout(PS_MainMenuScene *scene, VkDevice device)
{
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

    VkResult result = vkCreatePipelineLayout(device, &layoutInfo, NULL, &scene->pipelineLayout);
    PS_CHECK_VK_RESULT(result, "Failed to create main menu pipeline layout");
    return true;
}

static bool __psCreatePipeline(PS_MainMenuScene *scene, VkDevice device, VkRenderPass renderPass)
{
    PS_ASSERT(scene != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = psShaderModuleCreateFromAsset(device, "MainMenuVert");
    PS_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");
    VkShaderModule fragmentShaderModule = psShaderModuleCreateFromAsset(device, "MainMenuFrag");
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
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendStateInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = scene->pipelineLayout; // Use the updated layout
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &scene->pipeline);
    PS_CHECK_VK_RESULT(result, "Failed to create main menu graphics pipeline\n");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

bool psScenesMainMenuInit(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;
    PS_Vulkan *vulkan = &gameState->vulkan;

    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "TitleScreen", &scene->backgroundTexture));
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "TitleScreenNewGame", &scene->newGameButtonTexture));
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "TitleScreenContinue", &scene->continueButtonTexture));
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "TitleScreenOptions", &scene->optionsButtonTexture));
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "TitleScreenExit", &scene->exitButtonTexture));
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "MascotHope", &scene->mascotHopeTexture));
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "MascotCrush", &scene->mascotCrushTexture));
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "MascotMonster", &scene->mascotMonsterTexture));
    PS_CHECK(psVulkanImageLoadFromAsset(vulkan, "MascotFriend", &scene->mascotFriendTexture));

    PS_CHECK(__psCreateDescriptorSetLayout(scene, gameState->vulkan.device));
    PS_CHECK(__psCreateDescriptorSet(scene, gameState->vulkan.device, gameState->vulkan.descriptorPool));
    PS_CHECK(__psCreatePipelineLayout(scene, gameState->vulkan.device));
    PS_CHECK(__psCreatePipeline(scene, gameState->vulkan.device, gameState->vulkan.renderer.sceneFramebuffer.renderPass));

    scene->wasMouseClicked = false;
    scene->continueDisabled = true; // Disable continue button by default

    PS_CHECK(psRenderableTextCreate(gameState, &scene->titleText, "ShantellSansBold", psAssetShader_LoadingScreenFrag(), 48.0f));

    return true;
}

void psScenesMainMenuShutdown(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;
    PS_Vulkan *vulkan = &gameState->vulkan;

    psRenderableTextDestroy(gameState, &scene->titleText);

    // Destroy pipeline and layout first
    vkDestroyPipeline(vulkan->device, scene->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, scene->pipelineLayout, NULL);

    // Destroy descriptor set layout (descriptor set is implicitly freed with pool)
    vkDestroyDescriptorSetLayout(vulkan->device, scene->textureDescriptorSetLayout, NULL);

    // Destroy textures
    psVulkanImageDestroy(vulkan, &scene->backgroundTexture);
    psVulkanImageDestroy(vulkan, &scene->newGameButtonTexture);
    psVulkanImageDestroy(vulkan, &scene->continueButtonTexture);
    psVulkanImageDestroy(vulkan, &scene->optionsButtonTexture);
    psVulkanImageDestroy(vulkan, &scene->exitButtonTexture);
    psVulkanImageDestroy(vulkan, &scene->mascotHopeTexture);
    psVulkanImageDestroy(vulkan, &scene->mascotCrushTexture);
    psVulkanImageDestroy(vulkan, &scene->mascotMonsterTexture);
    psVulkanImageDestroy(vulkan, &scene->mascotFriendTexture);
}

bool psScenesMainMenuSwitch(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    return true;
}

bool psScenesMainMenuUpdate(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    PS_MainMenuScene *scene = &gameState->scene.mainMenuScene;

    float mx = gameState->input.mouseX * 0.5f + 0.5f;
    float my = gameState->input.mouseY * 0.5f + 0.5f;

    // Button layout constants
    const float buttonHeightUV = 0.1f;
    const float buttonCenterX = 0.5f;
    const float buttonCenterYStart = 0.45f;
    const float buttonSpacingY = 0.12f;

    // Compute button width in UV (accounting for window aspect & image aspect)
    float buttonImageAspect = (float)scene->newGameButtonTexture.width / scene->newGameButtonTexture.height;
    float windowAspect = (float)gameState->vulkan.renderer.sceneFramebuffer.width / (float)gameState->vulkan.renderer.sceneFramebuffer.height;
    float buttonWidthUV = buttonImageAspect * buttonHeightUV / windowAspect;

    for (int i = 0; i < PS_MAIN_MENU_BUTTON_COUNT; ++i)
    {
        float buttonCenterY = buttonCenterYStart - (buttonSpacingY * i);
        float buttonXMin = buttonCenterX - (buttonWidthUV / 2.0f);
        float buttonXMax = buttonCenterX + (buttonWidthUV / 2.0f);
        float buttonYMin = buttonCenterY - (buttonHeightUV / 2.0f);
        float buttonYMax = buttonCenterY + (buttonHeightUV / 2.0f);

        if (mx >= buttonXMin && mx <= buttonXMax && my >= buttonYMin && my <= buttonYMax)
        {
            scene->hoveredButton = i + 1;
            break;
        }
        else
        {
            scene->hoveredButton = PS_MAIN_MENU_BUTTON_NONE;
        }
    }

    // Check if the continue button is disabled then set hovered button to none
    if (scene->hoveredButton == PS_MAIN_MENU_BUTTON_CONTINUE && scene->continueDisabled)
    {
        scene->hoveredButton = PS_MAIN_MENU_BUTTON_NONE;
    }

    bool isMouseClicked = gameState->input.mouseButtonState[GLFW_MOUSE_BUTTON_LEFT];
    if (isMouseClicked && !scene->wasMouseClicked)
    {
        switch (scene->hoveredButton)
        {
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

bool psScenesMainMenuRender(PS_GameState *gameState)
{
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

    psRenderText(
        &gameState->vulkan,
        &gameState->fontRenderer,
        &scene->titleText,
        cmd,
        0.0, 0.0,
        1.0,
        0.1f, 0.2f, 0.3f, 1.0f
    );


    vkCmdEndRenderPass(cmd);


    return true;
}
