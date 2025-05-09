#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

// Texture samplers for all UI elements
layout(set = 0, binding = 0) uniform sampler2D backgroundTexture;
layout(set = 0, binding = 1) uniform sampler2D newGameButtonTexture;
layout(set = 0, binding = 2) uniform sampler2D continueButtonTexture;
layout(set = 0, binding = 3) uniform sampler2D optionsButtonTexture;
layout(set = 0, binding = 4) uniform sampler2D exitButtonTexture;
layout(set = 0, binding = 5) uniform sampler2D mascotHopeTexture;
layout(set = 0, binding = 6) uniform sampler2D mascotCrushTexture;
layout(set = 0, binding = 7) uniform sampler2D mascotMonsterTexture;
layout(set = 0, binding = 8) uniform sampler2D mascotFriendTexture;


struct PushConstantData {
    float windowWidth;
    float windowHeight;
    float time;
    float backgroundImageWidth;
    float backgroundImageHeight;
    float buttonImageWidth;
    float buttonImageHeight;
    float hoverScaleFactor;
    float hoverOffsetY;
    int hoveredButton;      // 0: None, 1: New Game, 2: Continue, 3: Options, 4: Exit
    int continueDisabled;   // 0: Enabled, 1: Disabled
};

layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;


const float buttonHeightUV = 0.1;
const float buttonCenterYStart = 0.55;
const float buttonSpacingY = 0.12;
const float buttonCenterX = 0.5;

const float mascotSizeUV = 0.15;

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);  
}


vec4 sampleRect(vec2 uv, vec2 center, vec2 size, sampler2D tex) {
    vec2 halfSize = size * 0.5;
    vec2 minBound = center - halfSize;
    vec2 maxBound = center + halfSize;

    if (uv.x >= minBound.x && uv.x <= maxBound.x && uv.y >= minBound.y && uv.y <= maxBound.y) {
        vec2 localUV = (uv - minBound) / size;
        return pow(texture(tex, localUV), vec4(1.3));
    }
    return vec4(0.0);
}

void main() {
    vec2 flippedUV = vec2(inUV.x, inUV.y);

    float screenAspect = pushConstants.data.windowWidth / pushConstants.data.windowHeight;

    float bgTargetAspect = pushConstants.data.backgroundImageWidth / pushConstants.data.backgroundImageHeight;
    vec2 bgScale = vec2(1.0, 1.0);
    vec2 bgOffset = vec2(0.0, 0.0);
    if (screenAspect > bgTargetAspect) {
        bgScale.x = bgTargetAspect / screenAspect;
        bgOffset.x = (1.0 - bgScale.x) * 0.5;
    } else {
        bgScale.y = screenAspect / bgTargetAspect;
        bgOffset.y = (1.0 - bgScale.y) * 0.5;
    }
    vec2 backgroundUV = (flippedUV - bgOffset) / bgScale;
    vec4 bgColor = vec4(0.0, 0.0, 0.0, 0.0);
    if (backgroundUV.x >= 0.0 && backgroundUV.x <= 1.0 && backgroundUV.y >= 0.0 && backgroundUV.y <= 1.0) {
        bgColor = texture(backgroundTexture, backgroundUV);
    }

    vec4 finalColor = bgColor;

    float time = pushConstants.data.time;

    float mascotAspect = 1.0; // Assuming square mascots
    float mascotWidthUV = mascotSizeUV * mascotAspect * (pushConstants.data.windowHeight / pushConstants.data.windowWidth);
    vec2 mascotDrawSize = vec2(mascotWidthUV, mascotSizeUV);

    vec2 hopePos = vec2(0.15, 0.8) + vec2(cos(time * 0.8 + 1.0) * 0.01, sin(time * 1.1 + 2.0) * 0.015);
    vec2 crushPos = vec2(0.85, 0.8) + vec2(cos(time * 0.9 - 0.5) * 0.015, sin(time * 1.0 - 1.0) * 0.01);
    vec2 monsterPos = vec2(0.1, 0.2) + vec2(cos(time * 1.2 + 3.0) * 0.01, sin(time * 0.7 + 0.5) * 0.015);
    vec2 friendPos = vec2(0.86, 0.3) + vec2(cos(time * 1.0 + 0.2) * 0.012, sin(time * 0.9 + 1.5) * 0.01);

    vec4 mascotColor = vec4(0.0);
    mascotColor = sampleRect(flippedUV, hopePos, mascotDrawSize, mascotHopeTexture);
    if (mascotColor.a > 0.0) { finalColor = mix(finalColor, mascotColor, mascotColor.a); }

    mascotColor = sampleRect(flippedUV, crushPos, mascotDrawSize, mascotCrushTexture);
    if (mascotColor.a > 0.0) { finalColor = mix(finalColor, mascotColor, mascotColor.a); }

    mascotColor = sampleRect(flippedUV, monsterPos, mascotDrawSize, mascotMonsterTexture);
    if (mascotColor.a > 0.0) { finalColor = mix(finalColor, mascotColor, mascotColor.a); }

    mascotColor = sampleRect(flippedUV, friendPos, mascotDrawSize, mascotFriendTexture);
    if (mascotColor.a > 0.0) { finalColor = mix(finalColor, mascotColor, mascotColor.a); }

    float buttonImageAspect = pushConstants.data.buttonImageWidth / pushConstants.data.buttonImageHeight;
    float windowAspect = pushConstants.data.windowWidth / pushConstants.data.windowHeight;
    float buttonWidthUV = buttonHeightUV * buttonImageAspect / windowAspect;
    vec2 correctedButtonSize = vec2(buttonWidthUV, buttonHeightUV);

   float buttonScale[4];
    buttonScale[0] = 1.0;
    buttonScale[1] = 1.0;
    buttonScale[2] = 1.0;
    buttonScale[3] = 1.0;

    vec2 buttonPositions[4];
    buttonPositions[0] = vec2(buttonCenterX, buttonCenterYStart + buttonSpacingY * 0.0);
    buttonPositions[1] = vec2(buttonCenterX, buttonCenterYStart + buttonSpacingY * 1.0);
    buttonPositions[2] = vec2(buttonCenterX, buttonCenterYStart + buttonSpacingY * 2.0);
    buttonPositions[3] = vec2(buttonCenterX, buttonCenterYStart + buttonSpacingY * 3.0);

    if (pushConstants.data.hoveredButton > 0 && pushConstants.data.hoveredButton <= 4) {
        int hoveredButton = pushConstants.data.hoveredButton - 1;
        buttonScale[hoveredButton] *= pushConstants.data.hoverScaleFactor;
        buttonPositions[hoveredButton].y += pushConstants.data.hoverOffsetY;
    }

    vec4 btnColor = vec4(0.0);

    btnColor = sampleRect(flippedUV, buttonPositions[0], correctedButtonSize * buttonScale[0], newGameButtonTexture);
    if (btnColor.a > 0.0) { finalColor = mix(finalColor, btnColor, btnColor.a); }

    btnColor = sampleRect(flippedUV, buttonPositions[1], correctedButtonSize * buttonScale[1], continueButtonTexture);
    if (btnColor.a > 0.0) { finalColor = mix(finalColor, btnColor, btnColor.a * (pushConstants.data.continueDisabled == 1 ? 0.5 : 1.0)); }

    btnColor = sampleRect(flippedUV, buttonPositions[2], correctedButtonSize * buttonScale[2], optionsButtonTexture);
    if (btnColor.a > 0.0) { finalColor = mix(finalColor, btnColor, btnColor.a); }

    btnColor = sampleRect(flippedUV, buttonPositions[3], correctedButtonSize * buttonScale[3], exitButtonTexture);
    if (btnColor.a > 0.0) { finalColor = mix(finalColor, btnColor, btnColor.a); }

    outColor = finalColor;
    outColor.rgb = pow(outColor.rgb, vec3(1.8));
    outColor.rgb = aces(outColor.rgb);
}