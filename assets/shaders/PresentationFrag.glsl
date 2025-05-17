#version 450

#pragma shader_stage(fragment)

layout (set=0, binding = 0) uniform sampler2D sceneFramebuffer;
layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

struct PushConstantData {
    float windowWidth;
    float windowHeight; 
    float framebufferWidth;
    float framebufferHeight;
    float sceneLoadingProgress;
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
    outColor = texture(sceneFramebuffer, inUV);
}