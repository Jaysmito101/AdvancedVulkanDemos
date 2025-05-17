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

    float imageWidth;
    float imageHeight;
    float pad0;
    float pad1;

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
    vec4 boxColor = vec4(pushConstants.data.colorR, pushConstants.data.colorG, pushConstants.data.colorB, pushConstants.data.colorA);

    if (p.x >= uiBoxMin.x && p.x <= uiBoxMax.x && p.y >= uiBoxMin.y && p.y <= uiBoxMax.y) {
        float boxWidthNorm = abs(uiBoxMax.x - uiBoxMin.x);
        float boxHeightNorm = abs(uiBoxMax.y - uiBoxMin.y);

        if (boxWidthNorm <= 1e-6 || boxHeightNorm <= 1e-6 || pushConstants.data.imageWidth <= 1e-6 || pushConstants.data.imageHeight <= 1e-6) {
            outColor = boxColor;
            return;
        }

        vec2 uvBox = vec2(0.0);  
        uvBox.x = (p.x - uiBoxMin.x) / boxWidthNorm;
        uvBox.y = (p.y - uiBoxMin.y) / boxHeightNorm;

        float boxPixelWidth = boxWidthNorm * pushConstants.data.frameWidth;
        float boxPixelHeight = boxHeightNorm * pushConstants.data.frameHeight;
        
        float boxAspect = boxPixelWidth / boxPixelHeight;
        float imageAspect = pushConstants.data.imageWidth / pushConstants.data.imageHeight;

        vec2 finalUV = uvBox;
        vec2 scale = vec2(1.0, 1.0);
        vec2 offset = vec2(0.0, 0.0);

        if (abs(boxAspect - imageAspect) < 1e-6) {
            // Aspect ratios are close enough, no scaling or offset needed.
        } else if (boxAspect > imageAspect) { 
            // Box is wider than image (needs letterboxing)
            // Image height fits box height, scale width.
            scale.x = imageAspect / boxAspect;
            offset.x = (1.0 - scale.x) / 2.0;
        } else { // boxAspect < imageAspect
            // Box is taller than image (needs pillarboxing)
            // Image width fits box width, scale height.
            scale.y = boxAspect / imageAspect;
            offset.y = (1.0 - scale.y) / 2.0;
        }
        
        finalUV = (uvBox - offset) / max(scale, vec2(1e-6));

        if (finalUV.x >= 0.0 && finalUV.x <= 1.0 && finalUV.y >= 0.0 && finalUV.y <= 1.0) {
            vec4 texColor = texture(customTexture, finalUV);
            outColor = mix(boxColor, texColor, texColor.a);
        } else {
            outColor = boxColor;
        }
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