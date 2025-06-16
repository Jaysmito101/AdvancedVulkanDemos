
#version 450

#pragma shader_stage(vertex)

layout(location = 0) out vec2 outUV;

struct ModelVertex {
    vec4 position;
    vec4 normal;
    vec4 texCoord;
    vec4 tangent;
    vec4 bitangent;
};

layout (set = 0, binding = 0, std430) readonly buffer VertexBuffer {
    ModelVertex vertices[];
};


struct PushConstantData {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;

    int vertexOffset;
    int vertexCount;
    int pad0;
    int pad1;
};


layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;

void main() {
    outUV = vec2(0.0, 0.0); // Initialize outUV to avoid warnings

    int vertexIndex = gl_VertexIndex + pushConstants.data.vertexOffset;
    vec4 vertexPosition = vertices[vertexIndex].position;
    vec4 position = transpose(pushConstants.data.projectionMatrix) * pushConstants.data.viewMatrix * pushConstants.data.modelMatrix * vec4(vertexPosition.xyz, 1.0);

    // Set the gl_Position for the vertex shader
    gl_Position = position;
}