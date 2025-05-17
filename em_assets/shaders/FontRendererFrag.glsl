#version 450

#pragma shader_stage(fragment)

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D fontAtlasImage;

struct PushConstantData {
    float fragmentBufferWidth;
    float fragmentBufferHeight;
    float scale;
    float opacity;
    float offsetX;
    float offsetY;
    float pxRange;
    float texSize;
    vec4 color;
};


layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(vec2 texCoord) {
    vec2 unitRange = vec2(pushConstants.data.pxRange / pushConstants.data.texSize);
    vec2 screenTexSize = vec2(1.0) / fwidth(texCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

void main() {
    vec2 txCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec3 msd = texture(fontAtlasImage, txCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange(txCoord) * (sd - 0.5);
    float alpha = smoothstep(0.0, 1.0, screenPxDistance +  0.5);
    outColor = vec4(pushConstants.data.color.rgb, pushConstants.data.color.a * pushConstants.data.opacity * alpha);
}