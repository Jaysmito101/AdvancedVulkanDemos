#version 450
#pragma shader_stage(fragment)

precision highp float;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

struct PushConstantData {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
};

layout(push_constant) uniform PushConstants {
   PushConstantData data;
} pushConstants;



void main() {
    outColor = vec4(1.0, 1.0, 0.0, 1.0); 
}

