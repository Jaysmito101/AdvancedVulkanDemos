#ifndef AVD_FONT_RENDERER_H
#define AVD_FONT_RENDERER_H

#include "core/avd_core.h"
#include "font/avd_font.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/avd_vulkan_image.h"
#include "vulkan/avd_vulkan_renderer.h"

struct AVD_FontRendererVertex;

#ifndef AVD_MAX_FONTS
#define AVD_MAX_FONTS 128
#endif

typedef struct AVD_Font {
    AVD_FontData fontData;
    AVD_VulkanImage fontAtlasImage;
    VkDescriptorSet fontDescriptorSet;
    VkDescriptorSetLayout fontDescriptorSetLayout;
} AVD_Font;

typedef struct AVD_RenderableText {
    size_t characterCount;
    size_t renderableVertexCount;
    AVD_VulkanBuffer vertexBuffer;
    struct AVD_FontRendererVertex *vertexBufferData;
    AVD_Font *font;
    char fontName[256];
    float charHeight;
    uint32_t numLines;

    float boundsMinX;
    float boundsMinY;
    float boundsMaxX;
    float boundsMaxY;
} AVD_RenderableText;

typedef struct AVD_FontRenderer {
    AVD_Font fonts[AVD_MAX_FONTS];
    size_t fontCount;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout fontDescriptorSetLayout;

    AVD_Vulkan *vulkan;
} AVD_FontRenderer;

bool avdFontCreate(AVD_FontData fontData, AVD_Vulkan *vulkan, AVD_Font *font);
void avdFontDestroy(AVD_Vulkan *vulkan, AVD_Font *font);

bool avdRenderableTextCreate(AVD_RenderableText *renderableText, AVD_FontRenderer *fontRenderer, AVD_Vulkan *vulkan, const char *fontName, const char *text, float charHeight);
bool avdRenderableTextUpdate(AVD_RenderableText *renderableText, AVD_FontRenderer *fontRenderer, AVD_Vulkan *vulkan, const char *text);
void avdRenderableTextDestroy(AVD_RenderableText *renderableText, AVD_Vulkan *vulkan);
void avdRenderableTextGetBounds(AVD_RenderableText *renderableText, float *minX, float *minY, float *maxX, float *maxY);
void avdRenderableTextGetSize(AVD_RenderableText *renderableText, float *width, float *height);

bool avdFontRendererInit(AVD_FontRenderer *fontRenderer, AVD_Vulkan *vulkan, AVD_VulkanRenderer *renderer);
void avdFontRendererShutdown(AVD_FontRenderer *fontRenderer);
bool avdFontRendererAddFontFromAsset(AVD_FontRenderer *fontRenderer, const char *asset);
bool avdFontRendererAddBasicFonts(AVD_FontRenderer *fontRenderer);
bool avdFontRendererHasFont(AVD_FontRenderer *fontRenderer, const char *fontName);
bool avdFontRendererGetFont(AVD_FontRenderer *fontRenderer, const char *fontName, AVD_Font **font);

void avdRenderText(AVD_Vulkan *vulkan, AVD_FontRenderer *fontRenderer, AVD_RenderableText *renderableText, VkCommandBuffer cmd, float x, float y, float scale, float r, float g, float b, float a, uint32_t framebufferWidth, uint32_t framebufferHeight);

#endif // AVD_FONT_RENDERER_H