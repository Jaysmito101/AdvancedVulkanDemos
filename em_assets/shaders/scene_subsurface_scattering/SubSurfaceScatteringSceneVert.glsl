
#version 450

#pragma shader_stage(vertex)

layout(location = 0) out vec2 outUV;

struct PushConstantData {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
};


layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;

void main() {
    outUV = vec2(0.0, 0.0); // Initialize outUV to avoid warnings

    // Calculate the position in clip space
    vec4 position = pushConstants.data.projectionMatrix * pushConstants.data.viewMatrix * pushConstants.data.modelMatrix * vec4(0.0, 0.0, 0.0, 1.0);
    
    // Set the gl_Position for the vertex shader
    gl_Position = position;
}