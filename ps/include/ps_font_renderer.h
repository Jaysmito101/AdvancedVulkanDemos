#ifndef PS_FONT_RENDERER_H
#define PS_FONT_RENDERER_H

#include "ps_common.h"

bool psFontCreate(PS_FontData fontData, PS_Vulkan* vulkan, PS_Font *font);
void psFontDestroy(PS_Vulkan* vulkan, PS_Font *font);

bool psRenderableTextCreate(PS_GameState* gameState, PS_RenderableText *renderableText, const char *fontName, const char *text, float charHeight);
bool psRenderableTextUpdate(PS_GameState* gameState, PS_RenderableText *renderableText, const char *text, float charHeight);
void psRenderableTextDestroy(PS_GameState* gameState, PS_RenderableText *renderableText);

bool psFontRendererInit(PS_GameState *gameState);
void psFontRendererShutdown(PS_GameState *gameState);
bool psFontRendererAddFontFromAsset(PS_GameState *gameState, const char *asset);
bool psFontRendererAddBasicFonts(PS_GameState *gameState);
bool psFontRendererHasFont(PS_FontRenderer *fontRenderer, const char *fontName);
bool psFontRendererGetFont(PS_FontRenderer *fontRenderer, const char *fontName, PS_Font **font);

void psRenderText(PS_Vulkan* vulkan, PS_FontRenderer* fontRenderer, PS_RenderableText *renderableText, VkCommandBuffer cmd, float x, float y, float scale, float r, float g, float b, float a);

#endif // PS_FONT_RENDERER_H