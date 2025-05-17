#include "font/avd_font_renderer.h"
#include "vulkan/avd_vulkan.h"
#include "shader/avd_shader.h"
#include "avd_asset.h"

typedef struct AVD_FontRendererPushConstants
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
} AVD_FontRendererPushConstants;

typedef struct AVD_FontRendererVertex
{
    float x;
    float y;
    float u;
    float v;
} AVD_FontRendererVertex;

static bool __avdCreateDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, AVD_VulkanImage *fontImage, VkDescriptorSet *descriptorSet)
{
    AVD_ASSERT(descriptorSet != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(descriptorPool != VK_NULL_HANDLE);
    AVD_ASSERT(descriptorSetLayout != VK_NULL_HANDLE);
    AVD_ASSERT(fontImage != NULL);

    // --- Allocate Descriptor Set ---
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSet);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate font descriptor set\n");

    // --- Update Descriptor Set ---
    VkWriteDescriptorSet descriptorWrite = {0};
    AVD_CHECK(avdWriteImageDescriptorSet(&descriptorWrite, *descriptorSet, 0, &fontImage->descriptorImageInfo));
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);

    return true;
}

static bool __avdCreatePipeline(AVD_FontRenderer *fr, VkDevice device, VkRenderPass renderPass)
{
    AVD_ASSERT(fr != NULL);
    AVD_ASSERT(device != VK_NULL_HANDLE);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);

    VkShaderModule vertexShaderModule = avdShaderModuleCreateFromAsset(device, "FontRendererVert");
    AVD_CHECK_VK_HANDLE(vertexShaderModule, "Failed to create vertex shader module\n");
    VkShaderModule fragmentShaderModule = avdShaderModuleCreateFromAsset(device, "FontRendererFrag");
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

    VkVertexInputBindingDescription vertexBindingDescription = {0};
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(AVD_FontRendererVertex);
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexAttributeDescription[2] = {0};
    vertexAttributeDescription[0].binding = 0;
    vertexAttributeDescription[0].location = 0;
    vertexAttributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescription[0].offset = offsetof(AVD_FontRendererVertex, x);

    vertexAttributeDescription[1].binding = 0;
    vertexAttributeDescription[1].location = 1;
    vertexAttributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescription[1].offset = offsetof(AVD_FontRendererVertex, u);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = AVD_ARRAY_COUNT(vertexAttributeDescription);
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription;

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = AVD_ARRAY_COUNT(shaderStages);
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
    AVD_CHECK_VK_RESULT(result, "Failed to create graphics pipeline\n");

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);

    return true;
}

static bool __avdSetCharQuad(AVD_FontRendererVertex *vertex, float ax, float ay, float bx, float by, float u0, float v0, float u1, float v1)
{
    AVD_ASSERT(vertex != NULL);

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

static bool __avdUpdateFontText(AVD_Vulkan *vulkan, AVD_RenderableText *renderableText, AVD_Font *font, const char *text, float charHeight)
{
    AVD_ASSERT(renderableText != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(font != NULL);

    renderableText->font = font;

    float cX = 0.0f;
    float cY = 0.0f;

    size_t indexOffset = 0;

    float lineHeight = font->fontData.atlas->metrics.lineHeight;
    float atlasWidth = (float)font->fontData.atlas->info.width;
    float atlasHeight = (float)font->fontData.atlas->info.height;

    renderableText->boundsMinX = 1000000.0f;
    renderableText->boundsMinY = 1000000.0f;
    renderableText->boundsMaxX = -1000000.0f;
    renderableText->boundsMaxY = -1000000.0f;
    renderableText->numLines = 1;

    for (size_t i = 0; i < renderableText->characterCount; i++)
    {
        uint32_t c = (uint32_t)text[i];
        if (c == '\n')
        {
            cX = 0.0f;
            cY += charHeight * lineHeight;
            renderableText->numLines++;
            continue;
        }

        if(c >= AVD_FONT_MAX_GLYPHS)
        {
            AVD_LOG("Font character out of range: %c [skipping]\n", c);
            continue;
        }

        if (c == ' ')
        {
            cX += font->fontData.atlas->glyphs[' '].advanceX * charHeight;
            continue;
        }

        if (c == '\r')
        {
            continue;
        }

        AVD_FontAtlasGlyph *glyph = &font->fontData.atlas->glyphs[c];
        AVD_CHECK(glyph->unicodeIndex == c); // TODO: Just for testing

        AVD_FontAtlasBounds *bounds = &glyph->atlasBounds;
        AVD_FontAtlasBounds *planeBounds = &glyph->planeBounds;

        float ax = cX + planeBounds->left * charHeight;
        float ay = cY - planeBounds->top * charHeight;
        float bx = cX + planeBounds->right * charHeight;
        float by = cY - planeBounds->bottom * charHeight;

        renderableText->boundsMinX = fminf(renderableText->boundsMinX, ax);
        renderableText->boundsMinY = fminf(renderableText->boundsMinY, ay);
        renderableText->boundsMaxX = fmaxf(renderableText->boundsMaxX, bx);
        renderableText->boundsMaxY = fmaxf(renderableText->boundsMaxY, by);

        cX += glyph->advanceX * charHeight;

        AVD_FontRendererVertex *vertex = &renderableText->vertexBufferData[indexOffset];
        AVD_CHECK(__avdSetCharQuad(vertex, ax, ay, bx, by,
                                 bounds->left / atlasWidth,
                                 bounds->top / atlasHeight,
                                 bounds->right / atlasWidth,
                                 bounds->bottom / atlasHeight));
        indexOffset += 6;
    }

    renderableText->renderableVertexCount = indexOffset;

    // update the vertex buffer with the new text
    AVD_VulkanBuffer *vertexBuffer = &renderableText->vertexBuffer;
    AVD_FontRendererVertex *mappedVertexBuffer = NULL;

    AVD_CHECK(avdVulkanBufferMap(vulkan, vertexBuffer, (void **)&mappedVertexBuffer));
    AVD_ASSERT(mappedVertexBuffer != NULL);
    memcpy(mappedVertexBuffer, renderableText->vertexBufferData, vertexBuffer->descriptorBufferInfo.range);
    avdVulkanBufferUnmap(vulkan, vertexBuffer);

    return true;
}

bool avdRenderableTextCreate(AVD_RenderableText *renderableText, AVD_FontRenderer* fontRenderer, AVD_Vulkan* vulkan, const char *fontName, const char *text, float charHeight)
{
    AVD_ASSERT(renderableText != NULL);
    AVD_ASSERT(fontName != NULL);
    AVD_ASSERT(fontRenderer != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(text != NULL);

    snprintf(renderableText->fontName, sizeof(renderableText->fontName), "%s", fontName);
    renderableText->characterCount = strlen(text);
    size_t currentSize = sizeof(AVD_FontRendererVertex) * 6 * renderableText->characterCount;

    // allocate the vertex buffer
    AVD_CHECK(avdVulkanBufferCreate(
        vulkan,
        &renderableText->vertexBuffer,
        currentSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT // so that we can map it anc easy ly update it
        ));

    // create the vertex buffer data
    renderableText->vertexBufferData = (AVD_FontRendererVertex *)malloc(currentSize);
    renderableText->charHeight = charHeight;

    // get the font data
    AVD_Font *font = NULL;
    AVD_CHECK(avdFontRendererGetFont(fontRenderer, fontName, &font));
    AVD_CHECK(__avdUpdateFontText(vulkan, renderableText, font, text, charHeight));

    return true;
}

bool avdRenderableTextUpdate(AVD_RenderableText *renderableText, AVD_FontRenderer* fontRenderer, AVD_Vulkan* vulkan, const char *text)
{
    AVD_ASSERT(renderableText != NULL);
    AVD_ASSERT(fontRenderer != NULL);
    AVD_ASSERT(vulkan != NULL);

    renderableText->characterCount = strlen(text);

    size_t currentSize = renderableText->vertexBuffer.descriptorBufferInfo.range;
    size_t newSize = sizeof(AVD_FontRendererVertex) * 6 * renderableText->characterCount;

    if (newSize > currentSize)
    {
        vkDeviceWaitIdle(vulkan->device);

        avdVulkanBufferDestroy(vulkan, &renderableText->vertexBuffer);
        // Reallocate the vertex buffer
        AVD_CHECK(avdVulkanBufferCreate(
            vulkan,
            &renderableText->vertexBuffer,
            newSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT // so that we can map it and easily update it
            ));

        free(renderableText->vertexBufferData);
        renderableText->vertexBufferData = (AVD_FontRendererVertex *)malloc(newSize);
    }
    // get the font data
    AVD_Font *font = NULL;
    AVD_CHECK(avdFontRendererGetFont(fontRenderer, renderableText->fontName, &font));

    // update the vertex buffer with the new text
    AVD_CHECK(__avdUpdateFontText(vulkan, renderableText, font, text, renderableText->charHeight));

    return true;
}

void avdRenderableTextDestroy(AVD_RenderableText *renderableText, AVD_Vulkan* vulkan)
{
    AVD_ASSERT(renderableText != NULL);
    AVD_ASSERT(vulkan != NULL);

    avdVulkanBufferDestroy(vulkan, &renderableText->vertexBuffer);
    free(renderableText->vertexBufferData);
}

void avdRenderableTextGetBounds(AVD_RenderableText *renderableText, float *minX, float *minY, float *maxX, float *maxY) 
{
    AVD_ASSERT(renderableText != NULL);
    AVD_ASSERT(minX != NULL);
    AVD_ASSERT(minY != NULL);
    AVD_ASSERT(maxX != NULL);
    AVD_ASSERT(maxY != NULL);

    *minX = renderableText->boundsMinX;
    *minY = renderableText->boundsMinY;
    *maxX = renderableText->boundsMaxX;
    *maxY = renderableText->boundsMaxY;
}

void avdRenderableTextGetSize(AVD_RenderableText *renderableText, float *width, float *height) 
{
    AVD_ASSERT(renderableText != NULL);
    AVD_ASSERT(width != NULL);
    AVD_ASSERT(height != NULL);

    *width = renderableText->boundsMaxX - renderableText->boundsMinX;
    *height = renderableText->boundsMaxY - renderableText->boundsMinY;
}

bool avdFontCreate(AVD_FontData fontData, AVD_Vulkan *vulkan, AVD_Font *font)
{
    AVD_ASSERT(font != NULL);
    font->fontData = fontData;
    AVD_CHECK(avdVulkanImageLoadFromMemory(vulkan, fontData.atlasData, fontData.atlasDataSize, &font->fontAtlasImage));
    AVD_CHECK(avdCreateDescriptorSetLayout(
        &font->fontDescriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT));

    AVD_CHECK(__avdCreateDescriptorSet(vulkan->device, font->fontDescriptorSetLayout, vulkan->descriptorPool, &font->fontAtlasImage, &font->fontDescriptorSet));
    return true;
}

void avdFontDestroy(AVD_Vulkan *vulkan, AVD_Font *font)
{
    AVD_ASSERT(font != NULL);
    avdVulkanImageDestroy(vulkan, &font->fontAtlasImage);
    vkDestroyDescriptorSetLayout(vulkan->device, font->fontDescriptorSetLayout, NULL);
}

bool avdFontRendererInit(AVD_FontRenderer* fontRenderer, AVD_Vulkan* vulkan, AVD_VulkanRenderer* renderer)
{
    AVD_ASSERT(fontRenderer != NULL);

    memset(&fontRenderer->fonts, 0, sizeof(fontRenderer->fonts));
    fontRenderer->fontCount = 0;

    fontRenderer->vulkan = vulkan;
    AVD_CHECK(avdCreateDescriptorSetLayout(
        &fontRenderer->fontDescriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT));
    AVD_CHECK(avdPipelineUtilsCreateGraphicsPipelineLayout(
        &fontRenderer->pipelineLayout,
        vulkan->device,
        &fontRenderer->fontDescriptorSetLayout, 1,
        sizeof(AVD_FontRendererPushConstants)));

    AVD_CHECK(__avdCreatePipeline(fontRenderer, vulkan->device, renderer->sceneFramebuffer.renderPass));

    return true;
}

void avdFontRendererShutdown(AVD_FontRenderer* fontRenderer)
{
    AVD_ASSERT(fontRenderer != NULL);

    for (size_t i = 0; i < fontRenderer->fontCount; ++i)
    {
        avdFontDestroy(fontRenderer->vulkan, &fontRenderer->fonts[i]);
    }

    vkDestroyPipeline(fontRenderer->vulkan->device, fontRenderer->pipeline, NULL);
    vkDestroyPipelineLayout(fontRenderer->vulkan->device, fontRenderer->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(fontRenderer->vulkan->device, fontRenderer->fontDescriptorSetLayout, NULL);
    fontRenderer->fontCount = 0;
    fontRenderer->vulkan = NULL;
}

bool avdFontRendererAddFontFromAsset(AVD_FontRenderer* fontRenderer, const char *asset)
{
    AVD_FontData fontData = {0};
    snprintf(fontData.name, sizeof(fontData.name), "%s", asset);
    fontData.atlas = (AVD_FontAtlas *)avdAssetFontMetrics(asset);
    fontData.atlasData = (uint8_t *)avdAssetFontAtlas(asset, &fontData.atlasDataSize);
    AVD_CHECK_MSG(fontData.atlasData != NULL, "Failed to load font asset\n");
    AVD_CHECK_MSG(fontData.atlas != NULL, "Failed to load font atlas\n");
    AVD_CHECK_MSG(fontRenderer->fontCount < AVD_MAX_FONTS, "Font count exceeded maximum limit\n");
    AVD_CHECK(avdFontCreate(fontData, fontRenderer->vulkan, &fontRenderer->fonts[fontRenderer->fontCount]));
    fontRenderer->fontCount += 1;
    return true;
}

bool avdFontRendererAddBasicFonts(AVD_FontRenderer* fontRenderer)
{
    AVD_ASSERT(fontRenderer != NULL);
    AVD_CHECK(avdFontRendererAddFontFromAsset(fontRenderer, "OpenSansRegular"));
    AVD_CHECK(avdFontRendererAddFontFromAsset(fontRenderer, "ShantellSansBold"));
    AVD_CHECK(avdFontRendererAddFontFromAsset(fontRenderer, "RampartOneRegular"));
    AVD_CHECK(avdFontRendererAddFontFromAsset(fontRenderer, "RubikGlitchRegular"));
    AVD_CHECK(avdFontRendererAddFontFromAsset(fontRenderer, "RobotoCondensedRegular"));
    return true;
}

bool avdFontRendererHasFont(AVD_FontRenderer *fontRenderer, const char *fontName)
{
    AVD_ASSERT(fontRenderer != NULL);
    for (size_t i = 0; i < fontRenderer->fontCount; ++i)
    {
        if (strcmp(fontRenderer->fonts[i].fontData.name, fontName) == 0)
        {
            return true;
        }
    }
    return false;
}

bool avdFontRendererGetFont(AVD_FontRenderer *fontRenderer, const char *fontName, AVD_Font **font)
{
    AVD_ASSERT(fontRenderer != NULL);
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

void avdRenderText(AVD_Vulkan *vulkan, AVD_FontRenderer *fontRenderer, AVD_RenderableText *renderableText, VkCommandBuffer cmd, float x, float y, float scale, float r, float g, float b, float a, uint32_t framebufferWidth, uint32_t framebufferHeight)
{
    AVD_ASSERT(renderableText != NULL);
    AVD_ASSERT(cmd != VK_NULL_HANDLE);
    AVD_ASSERT(fontRenderer != NULL);
    AVD_ASSERT(renderableText->font != NULL);

    AVD_FontRendererPushConstants pushConstants = {
        .frameBufferWidth = (float)framebufferWidth,
        .frameBufferHeight = (float)framebufferHeight,
        .scale = scale,
        .opacity = 1.0f,
        .offsetX = x,
        .offsetY = y + renderableText->charHeight - (renderableText->boundsMaxY - renderableText->boundsMinY),
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
    vkCmdDraw(cmd, (uint32_t)renderableText->renderableVertexCount, 1, 0, 0);
}