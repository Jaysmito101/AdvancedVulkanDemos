#version 450
#pragma shader_stage(fragment)

precision highp float;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

struct PushConstantData {
   float windowWidth;
};

layout(push_constant) uniform PushConstants {
   PushConstantData data;
} pushConstants;



void main() {
    vec2 uv = (inUV * 2.0 - 1.0);
    outColor = vec4(uv, 0.0, 1.0); 
}

