#version 450
#pragma shader_stage(vertex)
layout(location = 0) out vec2 outUV;

struct PushConstantData {
   float windowWidth;
   float windowHeight;
   float progress;
   float padding;
};

layout(push_constant) uniform PushConstants {
   PushConstantData data;
} pushConstants;



const vec2 positions[6] = vec2[] (
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

void main() {
    vec2 pos = positions[gl_VertexIndex];

    // adjust the rect to be a square quad in the center of the scree
    outUV = pos;
    pos = pos * 2.0 - 1.0;
    float aspectRatio = pushConstants.data.windowWidth / pushConstants.data.windowHeight;
    if (aspectRatio > 1.0)  {
        pos.x /= aspectRatio;
    } else {
        pos.y *= aspectRatio;
    }
    pos *= 1.4;
    gl_Position = vec4(pos, 0.0, 1.0);
}