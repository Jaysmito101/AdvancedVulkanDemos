
#version 450

#pragma shader_stage(vertex)

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBitangent;
layout(location = 4) out vec4 outPosition;

struct ModelVertex {
    vec4 position;
    vec4 normal;
    vec4 texCoord;
    vec4 tangent;
    vec4 bitangent;
};

layout(set = 0, binding = 0, std430) readonly buffer VertexBuffer
{
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

layout(push_constant) uniform PushConstants
{
    PushConstantData data;
}
pushConstants;

void main()
{
    outUV = vec2(0.0, 0.0); // Initialize outUV to avoid warnings

    int vertexIndex = gl_VertexIndex + pushConstants.data.vertexOffset;

    mat4 viewModel    = pushConstants.data.viewMatrix * pushConstants.data.modelMatrix;
    mat4 projection   = pushConstants.data.projectionMatrix;
    mat3 normalMatrix = transpose(inverse(mat3(pushConstants.data.modelMatrix)));

    vec4 vertexPosition = vertices[vertexIndex].position;
    vec4 position       = projection * viewModel * vec4(vertexPosition.xyz, 1.0);

    // Set the output variables
    outNormal    = normalMatrix * vertices[vertexIndex].normal.xyz;
    outTangent   = normalMatrix * vertices[vertexIndex].tangent.xyz;
    outBitangent = normalMatrix * vertices[vertexIndex].bitangent.xyz;
    outPosition  = position;

    // Set the gl_Position for the vertex shader
    gl_Position = position;
}