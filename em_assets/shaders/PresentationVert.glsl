
#version 450

#pragma shader_stage(vertex)

layout (location = 0) out vec2 outUV;

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

const vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

void main() {
    vec2 pos = positions[gl_VertexIndex];

    outUV = pos;

    pos = pos * 2.0 - 1.0;

    float windowAspect = pushConstants.data.windowWidth / pushConstants.data.windowHeight;
    float framebufferAspect = pushConstants.data.framebufferWidth / pushConstants.data.framebufferHeight;
    if (windowAspect > framebufferAspect) {
        pos.x *= framebufferAspect / windowAspect;
    } else {
        pos.y *= windowAspect / framebufferAspect;
    }
    gl_Position = vec4(pos, 0.0, 1.0);
}