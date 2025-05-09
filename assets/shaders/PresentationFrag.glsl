#version 450

#pragma shader_stage(fragment)

layout (set=0, binding = 0) uniform sampler2D sceneFramebuffer;
layout (set=1, binding = 0) uniform sampler2D iconTexture;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

struct PushConstantData {
    float windowWidth;
    float windowHeight; 
    float framebufferWidth;
    float framebufferHeight;
    float circleRadius;
    float iconWidth;
    float iconHeight;
    float time;
};


layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() 
{
    vec2 ndc = inUV * 2.0 - 1.0; // Convert UV to NDC [-1, 1]

    float framebufferAspect = pushConstants.data.framebufferWidth / pushConstants.data.framebufferHeight;
    ndc.x *= framebufferAspect;

    float dist = length(ndc);

    vec3 overlayColor = vec3(215.0 / 255.0, 204.0 / 255.0, 246.0 / 255.0);
    float gAngle = pushConstants.data.time * 0.1;
    mat2 rotationMatrix = mat2(cos(gAngle), -sin(gAngle), sin(gAngle), cos(gAngle));
    vec2 rotatedNdc = rotationMatrix * ndc;
    vec3 uvRotFactor = mix(rotatedNdc.xyy, rotatedNdc.yxy, smoothstep(-0.5, 0.5, dist)) * 0.5 + 0.5;
    overlayColor = pow(overlayColor, uvRotFactor);

    outColor = texture(sceneFramebuffer, inUV);
    float overlayAlpha = smoothstep(pushConstants.data.circleRadius, pushConstants.data.circleRadius + 0.01, dist);
    outColor.rgb = mix(outColor.rgb, overlayColor, overlayAlpha);
    outColor.a = 1.0;
    if (dist <= 0.155 && pushConstants.data.circleRadius <= 0.1) {
        float timefactor = smoothstep(-1.0, 1.0, sin(pushConstants.data.time * 20.0));
        float angle = sin(timefactor) * 0.2;
        float cosAngle = cos(angle);
        float sinAngle = sin(angle);
        mat2 rotationMatrix = mat2(cosAngle, -sinAngle, sinAngle, cosAngle);
        vec2 rotatedNdc = rotationMatrix * ndc;
        vec2 iconUV = rotatedNdc * 6.5 * 0.5 + 0.5;
        vec4 iconColor = texture(iconTexture, iconUV);
        float opacity = 1.0 - smoothstep(0.1, 0.15, pushConstants.data.circleRadius);
        outColor = mix(outColor, iconColor, iconColor.a * opacity);
    }
}