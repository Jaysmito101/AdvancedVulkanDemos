#version 450

#pragma shader_stage(fragment)

layout (set=0, binding = 0) uniform sampler2D customTexture;
layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

#define UI_TYPE_NONE 0
#define UI_TYPE_RECT 1

struct PushConstantData {
    int type;
    float width;
    float height;
    float radius;
    
    float offsetX;
    float offsetY;
    float frameWidth;
    float frameHeight;

    float uiBoxMinX;
    float uiBoxMinY;
    float uiBoxMaxX;
    float uiBoxMaxY;

    float colorR;
    float colorG;
    float colorB;
    float colorA;
};


layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;

void uiRect() {
    vec2 p = inUV; 
    vec2 uiBoxMin = vec2(pushConstants.data.uiBoxMinX, pushConstants.data.uiBoxMinY);
    vec2 uiBoxMax = vec2(pushConstants.data.uiBoxMaxX, pushConstants.data.uiBoxMaxY);

    // draw a box from uiBoxMin to uiBoxMax using the color
    if (p.x >= uiBoxMin.x && p.x <= uiBoxMax.x && p.y >= uiBoxMin.y && p.y <= uiBoxMax.y) {
        outColor = vec4(pushConstants.data.colorR, pushConstants.data.colorG, pushConstants.data.colorB, pushConstants.data.colorA);
    }
}

void main() 
{
    outColor = vec4(0.0); // Default to transparent

    if (pushConstants.data.type == UI_TYPE_RECT) {
        uiRect();
    } else {
        outColor = texture(customTexture, inUV);
    }
}