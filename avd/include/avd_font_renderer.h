#ifndef AVD_FONT_RENDERER_H
#define AVD_FONT_RENDERER_H

#include "avd_common.h"

bool avdFontCreate(AVD_FontData fontData, AVD_Vulkan* vulkan, AVD_Font *font);
void avdFontDestroy(AVD_Vulkan* vulkan, AVD_Font *font);

bool avdRenderableTextCreate(AVD_GameState* gameState, AVD_RenderableText *renderableText, const char *fontName, const char *text, float charHeight);
bool avdRenderableTextUpdate(AVD_GameState* gameState, AVD_RenderableText *renderableText, const char *text);
void avdRenderableTextDestroy(AVD_GameState* gameState, AVD_RenderableText *renderableText);
void avdRenderableTextGetBounds(AVD_RenderableText *renderableText, float *minX, float *minY, float *maxX, float *maxY);
void avdRenderableTextGetSize(AVD_RenderableText *renderableText, float *width, float *height);

bool avdFontRendererInit(AVD_FontRenderer* fontRenderer, AVD_Vulkan* vulkan);
void avdFontRendererShutdown(AVD_FontRenderer* fontRenderer);
bool avdFontRendererAddFontFromAsset(AVD_FontRenderer* fontRenderer, const char *asset);
bool avdFontRendererAddBasicFonts(AVD_FontRenderer* fontRenderer);
bool avdFontRendererHasFont(AVD_FontRenderer *fontRenderer, const char *fontName);
bool avdFontRendererGetFont(AVD_FontRenderer *fontRenderer, const char *fontName, AVD_Font **font);

void avdRenderText(AVD_Vulkan* vulkan, AVD_FontRenderer* fontRenderer, AVD_RenderableText *renderableText, VkCommandBuffer cmd, float x, float y, float scale, float r, float g, float b, float a);

#endif // AVD_FONT_RENDERER_H