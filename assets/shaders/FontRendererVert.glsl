
#version 450

#pragma shader_stage(vertex)

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

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

void main() {
    vec2 offset = vec2(pushConstants.data.offsetX, pushConstants.data.offsetY);
    vec2 pos = (inPosition + offset) * pushConstants.data.scale;
    pos /= vec2(pushConstants.data.fragmentBufferWidth, pushConstants.data.fragmentBufferHeight);
    pos = pos * 2.0 - 1.0;
    fragTexCoord = inTexCoord;
    gl_Position = vec4(pos, 0.0, 1.0);
}