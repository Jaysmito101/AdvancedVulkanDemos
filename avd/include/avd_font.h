#ifndef AVD_FONT_H
#define AVD_FONT_H

#include "avd_base.h"

#ifndef AVD_FONT_MAX_GLYPHS
#define AVD_FONT_MAX_GLYPHS 4096
#endif

typedef struct AVD_FontAtlasInfo {
    float distanceRange;
    float distanceRangeMiddle;
    float size;
    uint32_t width;
    uint32_t height;
} AVD_FontAtlasInfo;

typedef struct AVD_FontAtlasMetrics {
    float emSize;
    float lineHeight;
    float ascender;
    float descender;
    float underlineY;
    float underlineThickness;
} AVD_FontAtlasMetrics;

typedef struct AVD_FontAtlasBounds {
    float left;
    float bottom;
    float right;
    float top;
} AVD_FontAtlasBounds;

typedef struct AVD_FontAtlasGlyph {
    uint32_t unicodeIndex;
    float advanceX;
    AVD_FontAtlasBounds planeBounds;
    AVD_FontAtlasBounds atlasBounds;
} AVD_FontAtlasGlyph;

typedef struct AVD_FontAtlas {
    AVD_FontAtlasInfo info;
    AVD_FontAtlasMetrics metrics;
    AVD_FontAtlasGlyph glyphs[AVD_FONT_MAX_GLYPHS];
    uint32_t glyphCount;
} AVD_FontAtlas;

typedef struct AVD_FontData {
    char name[256];
    AVD_FontAtlas* atlas;
    uint8_t *atlasData;
    size_t atlasDataSize;
} AVD_FontData;

#endif // AVD_FONT_H