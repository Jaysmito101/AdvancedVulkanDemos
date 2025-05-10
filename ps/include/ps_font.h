#ifndef PS_FONT_H
#define PS_FONT_H

#include "ps_base.h"

#ifndef PS_FONT_MAX_GLYPHS
#define PS_FONT_MAX_GLYPHS 4096
#endif

typedef struct PS_FontAtlasInfo {
    float distanceRange;
    float distanceRangeMiddle;
    float size;
    uint32_t width;
    uint32_t height;
} PS_FontAtlasInfo;

typedef struct PS_FontAtlasMetrics {
    float emSize;
    float lineHeight;
    float ascender;
    float descender;
    float underlineY;
    float underlineThickness;
} PS_FontAtlasMetrics;

typedef struct PS_FontAtlasBounds {
    float left;
    float right;
    float top;
    float bottom;
} PS_FontAtlasBounds;

typedef struct PS_FontAtlasGlyph {
    uint32_t unicodeIndex;
    float advanceX;
    PS_FontAtlasBounds planeBounds;
    PS_FontAtlasBounds atlasBounds;
} PS_FontAtlasGlyph;

typedef struct PS_FontAtlas {
    PS_FontAtlasInfo info;
    PS_FontAtlasMetrics metrics;
    PS_FontAtlasGlyph glyphs[PS_FONT_MAX_GLYPHS];
    uint32_t glyphCount;
} PS_FontAtlas;

typedef struct PS_FontData {
    char name[256];
    PS_FontAtlas* atlas;
    uint8_t *atlasData;
    size_t atlasDataSize;
} PS_FontData;

#endif // PS_FONT_H