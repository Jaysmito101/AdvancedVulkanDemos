#include "ps_font_renderer.h"
#include "ps_vulkan.h"
#include "ps_shader.h"
#include "ps_scenes.h"

typedef struct PS_FontRendererPushConstants
{
    float frameBufferWidth;
    float frameBufferHeight;
    float scale;
    float opacity;
    float offsetX;
    float offsetY;
    float pxRange;
    float texSize;
    float colorR;
    float colorG;
    float colorB;
    float colorA;
} PS_FontRendererPushConstants;

typedef struct PS_FontRendererVertex
{
    float x;
    float y;
    float u;
    float v;
} PS_FontRendererVertex;

static bool __psCreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout *descriptorSetLayout)
{
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(descriptorSetLayout != NULL);

    // --- Font Descriptor Set Layout ---
    // (1) -> Font Atlas Image
    VkDescriptorSetLayoutBinding fontLayoutBindings[1] = {0};
    fontLayoutBindings[0].binding = 0;
    fontLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    fontLayoutBindings[0].descriptorCount = 1;
    fontLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo fontLayoutInfo = {0};
    fontLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    fontLayoutInfo.bindingCount = PS_ARRAY_COUNT(fontLayoutBindings);
    fontLayoutInfo.pBindings = fontLayoutBindings;

    VkResult result = vkCreateDescriptorSetLayout(device, &fontLayoutInfo, NULL, descriptorSetLayout);
    PS_CHECK_VK_RESULT(result, "Failed to create font descriptor set layout\n");

    return true;
}

static bool __psCreateDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, PS_VulkanImage *fontImage, VkDescriptorSet *descriptorSet)
{
    PS_ASSERT(descriptorSet != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(descriptorPool != VK_NULL_HANDLE);
    PS_ASSERT(descriptorSetLayout != VK_NULL_HANDLE);
    PS_ASSERT(fontImage != NULL);

    // --- Allocate Descriptor Set ---
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSet);
    PS_CHECK_VK_RESULT(result, "Failed to allocate font descriptor set\n");

    // --- Update Descriptor Set ---
    VkWriteDescriptorSet descriptorWrite = {0};
    PS_CHECK(psWriteImageDescriptorSet(&descriptorWrite, *descriptorSet, 0, &fontImage->descriptorImageInfo));
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);

    return true;
}

static bool __psCreatePipelineLayout(PS_FontRenderer *fr, VkDevice device)
{
    PS_ASSERT(fr != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);

    VkPushConstantRange pushConstantRanges[1] = {0};
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(PS_FontRendererPushConstants);

    const VkDescriptorSetLayout setLayouts[1] = {
        fr->fontDescriptorSetLayout,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = PS_ARRAY_COUNT(setLayouts);
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    pipelineLayoutInfo.pushConstantRangeCount = PS_ARRAY_COUNT(pushConstantRanges);

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &fr->pipelineLayout);
    PS_CHECK_VK_RESULT(result, "Failed to create pipeline layout\n");
    return true;
}

static bool __psCreatePipeline(PS_FontRenderer *fr, VkDevice device, VkRenderPass renderPass)
{
    PS_ASSERT(fr != NULL);
    PS_ASSERT(device != VK_NULL_HANDLE);
    PS_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = psShaderModuleCreateFromAsset(device, "FontRendererVert");
    PS_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");
    VkShaderModule fragmentShaderModule = psShaderModuleCreateFromAsset(device, "FontRendererFrag");
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

    VkVertexInputBindingDescription vertexBindingDescription = {0};
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(PS_FontRendererVertex);
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexAttributeDescription[2] = {0};
    vertexAttributeDescription[0].binding = 0;
    vertexAttributeDescription[0].location = 0;
    vertexAttributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescription[0].offset = offsetof(PS_FontRendererVertex, x);

    vertexAttributeDescription[1].binding = 0;
    vertexAttributeDescription[1].location = 1;
    vertexAttributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescription[1].offset = offsetof(PS_FontRendererVertex, u);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = PS_ARRAY_COUNT(vertexAttributeDescription);
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription;

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = PS_ARRAY_COUNT(shaderStages);
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.layout = fr->pipelineLayout;
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

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &fr->pipeline);
    PS_CHECK_VK_RESULT(result, "Failed to create graphics pipeline\n");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

static bool __psSetCharQuad(PS_FontRendererVertex *vertex, float ax, float ay, float bx, float by, float u0, float v0, float u1, float v1)
{
    PS_ASSERT(vertex != NULL);

    vertex[0].x = ax;
    vertex[0].y = ay;
    vertex[0].u = u0;
    vertex[0].v = v0;

    vertex[1].x = bx;
    vertex[1].y = ay;
    vertex[1].u = u1;
    vertex[1].v = v0;

    vertex[2].x = bx;
    vertex[2].y = by;
    vertex[2].u = u1;
    vertex[2].v = v1;

    vertex[3].x = ax;
    vertex[3].y = by;
    vertex[3].u = u0;
    vertex[3].v = v1;

    vertex[4] = vertex[0];
    vertex[5] = vertex[2];

    return true;
}

static bool __psUpdateFontText(PS_Vulkan *vulkan, PS_RenderableText *renderableText, PS_Font *font, const char *text, float charHeight)
{
    PS_ASSERT(renderableText != NULL);
    PS_ASSERT(vulkan != NULL);
    PS_ASSERT(font != NULL);

    renderableText->font = font;

    float cX = 0.0f;
    float cY = 0.0f;

    size_t indexOffset = 0;

    float lineHeight = font->fontData.atlas->metrics.lineHeight;
    float atlasWidth = (float)font->fontData.atlas->info.width;
    float atlasHeight = (float)font->fontData.atlas->info.height;

    for (size_t i = 0; i < renderableText->characterCount; i++)
    {
        uint32_t c = (uint32_t)text[i];
        if (c == '\n')
        {
            cX = 0.0f;
            cY += charHeight * lineHeight;
            continue;
        }

        PS_CHECK_MSG(c < PS_FONT_MAX_GLYPHS, "Character %c is out of range\n", c);

        if (c == ' ')
        {
            cX += font->fontData.atlas->glyphs[' '].advanceX * charHeight;
            continue;
        }

        if (c == '\r')
        {
            continue;
        }

        PS_FontAtlasGlyph *glyph = &font->fontData.atlas->glyphs[c];
        PS_CHECK(glyph->unicodeIndex == c); // TODO: Just for testing

        PS_FontAtlasBounds *bounds = &glyph->atlasBounds;
        PS_FontAtlasBounds *planeBounds = &glyph->planeBounds;

        float ax = cX + planeBounds->left * charHeight;
        float ay = cY - planeBounds->top * charHeight;
        float bx = cX + planeBounds->right * charHeight;
        float by = cY - planeBounds->bottom * charHeight;

        cX += glyph->advanceX * charHeight;

        PS_FontRendererVertex *vertex = &renderableText->vertexBufferData[indexOffset];
        PS_CHECK(__psSetCharQuad(vertex, ax, ay, bx, by,
                                 bounds->left / atlasWidth,
                                 bounds->top / atlasHeight,
                                 bounds->right / atlasWidth,
                                 bounds->bottom / atlasHeight));
        indexOffset += 6;
    }

    // update the vertex buffer with the new text
    PS_VulkanBuffer *vertexBuffer = &renderableText->vertexBuffer;
    PS_FontRendererVertex *mappedVertexBuffer = NULL;

    PS_CHECK(psVulkanBufferMap(vulkan, vertexBuffer, (void **)&mappedVertexBuffer));
    PS_ASSERT(mappedVertexBuffer != NULL);
    memcpy(mappedVertexBuffer, renderableText->vertexBufferData, vertexBuffer->descriptorBufferInfo.range);
    psVulkanBufferUnmap(vulkan, vertexBuffer);

    return true;
}

bool psRenderableTextCreate(PS_GameState *gameState, PS_RenderableText *renderableText, const char *fontName, const char *text, float charHeight)
{
    PS_ASSERT(renderableText != NULL);
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(fontName != NULL);
    PS_ASSERT(text != NULL);
    snprintf(renderableText->fontName, sizeof(renderableText->fontName), "%s", fontName);

    PS_Vulkan *vulkan = &gameState->vulkan;
    PS_FontRenderer *fontRenderer = &gameState->fontRenderer;

    renderableText->characterCount = strlen(text);
    size_t currentSize = sizeof(PS_FontRendererVertex) * 6 * renderableText->characterCount;

    // allocate the vertex buffer
    PS_CHECK(psVulkanBufferCreate(
        vulkan,
        &renderableText->vertexBuffer,
        currentSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT // so that we can map it anc easy ly update it
        ));

    // create the vertex buffer data
    renderableText->vertexBufferData = (PS_FontRendererVertex *)malloc(currentSize);

    // get the font data
    PS_Font *font = NULL;
    PS_CHECK(psFontRendererGetFont(fontRenderer, fontName, &font));
    PS_CHECK(__psUpdateFontText(vulkan, renderableText, font, text, charHeight));

    return true;
}

bool psRenderableTextUpdate(PS_GameState *gameState, PS_RenderableText *renderableText, const char *text, float charHeight)
{
    PS_ASSERT(renderableText != NULL);
    PS_ASSERT(gameState != NULL);

    PS_Vulkan *vulkan = &gameState->vulkan;
    PS_FontRenderer *fontRenderer = &gameState->fontRenderer;

    renderableText->characterCount = strlen(text);

    size_t currentSize = renderableText->vertexBuffer.descriptorBufferInfo.range;
    size_t newSize = sizeof(PS_FontRendererVertex) * 6 * renderableText->characterCount;

    if (newSize > currentSize)
    {
        psVulkanBufferDestroy(vulkan, &renderableText->vertexBuffer);
        // Reallocate the vertex buffer
        PS_CHECK(psVulkanBufferCreate(
            vulkan,
            &renderableText->vertexBuffer,
            newSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT // so that we can map it and easily update it
            ));

        free(renderableText->vertexBufferData);
        renderableText->vertexBufferData = (PS_FontRendererVertex *)malloc(newSize);
    }
    // get the font data
    PS_Font *font = NULL;
    PS_CHECK(psFontRendererGetFont(fontRenderer, renderableText->fontName, &font));

    // update the vertex buffer with the new text
    PS_CHECK(__psUpdateFontText(vulkan, renderableText, font, text, charHeight));

    return true;
}

void psRenderableTextDestroy(PS_GameState *gameState, PS_RenderableText *renderableText)
{
    PS_ASSERT(renderableText != NULL);
    PS_ASSERT(gameState != NULL);

    psVulkanBufferDestroy(&gameState->vulkan, &renderableText->vertexBuffer);
    free(renderableText->vertexBufferData);
}

bool psFontCreate(PS_FontData fontData, PS_Vulkan *vulkan, PS_Font *font)
{
    PS_ASSERT(font != NULL);
    font->fontData = fontData;
    PS_CHECK(psVulkanImageLoadFromMemory(vulkan, fontData.atlasData, fontData.atlasDataSize, &font->fontAtlasImage));
    PS_CHECK(__psCreateDescriptorSetLayout(vulkan->device, &font->fontDescriptorSetLayout));
    PS_CHECK(__psCreateDescriptorSet(vulkan->device, font->fontDescriptorSetLayout, vulkan->descriptorPool, &font->fontAtlasImage, &font->fontDescriptorSet));
    return true;
}

void psFontDestroy(PS_Vulkan *vulkan, PS_Font *font)
{
    PS_ASSERT(font != NULL);
    psVulkanImageDestroy(vulkan, &font->fontAtlasImage);
    vkDestroyDescriptorSetLayout(vulkan->device, font->fontDescriptorSetLayout, NULL);
}

bool psFontRendererInit(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    PS_FontRenderer *fontRenderer = &gameState->fontRenderer;
    memset(&fontRenderer->fonts, 0, sizeof(fontRenderer->fonts));
    fontRenderer->fontCount = 0;

    PS_CHECK(__psCreateDescriptorSetLayout(gameState->vulkan.device, &fontRenderer->fontDescriptorSetLayout));
    PS_CHECK(__psCreatePipelineLayout(fontRenderer, gameState->vulkan.device));
    PS_CHECK(__psCreatePipeline(fontRenderer, gameState->vulkan.device, gameState->vulkan.renderer.sceneFramebuffer.renderPass));

    return true;
}

void psFontRendererShutdown(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    PS_FontRenderer *fontRenderer = &gameState->fontRenderer;

    for (size_t i = 0; i < fontRenderer->fontCount; ++i)
    {
        psFontDestroy(&gameState->vulkan, &fontRenderer->fonts[i]);
    }

    vkDestroyPipeline(gameState->vulkan.device, fontRenderer->pipeline, NULL);
    vkDestroyPipelineLayout(gameState->vulkan.device, fontRenderer->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(gameState->vulkan.device, fontRenderer->fontDescriptorSetLayout, NULL);
    fontRenderer->fontCount = 0;
}

bool psFontRendererAddFontFromAsset(PS_GameState *gameState, const char *asset)
{
    PS_FontData fontData = {0};
    snprintf(fontData.name, sizeof(fontData.name), "%s", asset);
    fontData.atlas = (PS_FontAtlas *)psAssetFontMetrics(asset);
    fontData.atlasData = (uint8_t *)psAssetFontAtlas(asset, &fontData.atlasDataSize);
    PS_CHECK_MSG(fontData.atlasData != NULL, "Failed to load font asset\n");
    PS_CHECK_MSG(fontData.atlas != NULL, "Failed to load font atlas\n");
    PS_CHECK_MSG(gameState->fontRenderer.fontCount < PS_MAX_FONTS, "Font count exceeded maximum limit\n");
    PS_CHECK(psFontCreate(fontData, &gameState->vulkan, &gameState->fontRenderer.fonts[gameState->fontRenderer.fontCount]));
    gameState->fontRenderer.fontCount += 1;
    return true;
}

bool psFontRendererAddBasicFonts(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    PS_CHECK(psFontRendererAddFontFromAsset(gameState, "OpenSansRegular"));
    PS_CHECK(psFontRendererAddFontFromAsset(gameState, "ShantellSansBold"));
    return true;
}

bool psFontRendererHasFont(PS_FontRenderer *fontRenderer, const char *fontName)
{
    PS_ASSERT(fontRenderer != NULL);
    for (size_t i = 0; i < fontRenderer->fontCount; ++i)
    {
        if (strcmp(fontRenderer->fonts[i].fontData.name, fontName) == 0)
        {
            return true;
        }
    }
    return false;
}

bool psFontRendererGetFont(PS_FontRenderer *fontRenderer, const char *fontName, PS_Font **font)
{
    PS_ASSERT(fontRenderer != NULL);
    for (size_t i = 0; i < fontRenderer->fontCount; ++i)
    {
        if (strcmp(fontRenderer->fonts[i].fontData.name, fontName) == 0)
        {
            *font = &fontRenderer->fonts[i];
            return true;
        }
    }
    *font = NULL;
    return false;
}

void psRenderText(PS_Vulkan *vulkan, PS_FontRenderer *fontRenderer, PS_RenderableText *renderableText, VkCommandBuffer cmd, float x, float y, float scale, float r, float g, float b, float a)
{
    PS_ASSERT(renderableText != NULL);
    PS_ASSERT(cmd != VK_NULL_HANDLE);
    PS_ASSERT(fontRenderer != NULL);
    PS_ASSERT(renderableText->font != NULL);

    
    PS_FontRendererPushConstants pushConstants = {
        .frameBufferWidth = (float)vulkan->renderer.sceneFramebuffer.width,
        .frameBufferHeight = (float)vulkan->renderer.sceneFramebuffer.height,
        .scale = scale,
        .opacity = 1.0f,
        .offsetX = x,
        .offsetY = y,
        .pxRange = renderableText->font->fontData.atlas->info.distanceRange,
        .texSize = (float)renderableText->font->fontData.atlas->info.width,
        .colorR = r,
        .colorG = g,
        .colorB = b,
        .colorA = a,
    };

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fontRenderer->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fontRenderer->pipelineLayout,
                            0,
                            1,
                            &renderableText->font->fontDescriptorSet,
                            0, NULL);
    vkCmdPushConstants(cmd, fontRenderer->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

    vkCmdBindVertexBuffers(cmd, 0, 1, &renderableText->vertexBuffer.descriptorBufferInfo.buffer, (VkDeviceSize[]){0});
    vkCmdDraw(cmd, (uint32_t)renderableText->characterCount * 6, 1, 0, 0);
    // vkCmdDraw(cmd, 6, 1, 0, 0);
}