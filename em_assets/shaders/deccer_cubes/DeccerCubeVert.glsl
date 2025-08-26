
#version 450

#pragma shader_stage(vertex)

#include "DeccerCubeCommon"

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outPosition;

layout(set = 0, binding = 0, std430) readonly buffer VertexBuffer
{
    ModelVertex vertices[];
};

layout(set = 0, binding = 1, std430) readonly buffer IndexBuffer
{
    uint indices[];
};

layout(push_constant) uniform PushConstants
{
    UberPushConstantData data;
}
pushConstants;

uint getVertexIndex() {
    return indices[gl_VertexIndex + pushConstants.data.vertexOffset];
}

vec4 samplePosition()
{
    uint index = getVertexIndex();
    return vec4(vertices[index].vx, vertices[index].vy, vertices[index].vz, 1.0);
}

vec2 sampleTextureCoords()
{
    uint index = getVertexIndex();
    return vec2(vertices[index].tu, vertices[index].tv);
}

vec3 sampleNormal()
{
    uint index = getVertexIndex();
    vec3 normal;
    vec4 tangent;
    unpackTBN(vertices[index].np, uint(vertices[index].tp), normal, tangent);
    return normalize(normal);
}

void main()
{
    mat4 viewModelMatrix  = pushConstants.data.viewModelMatrix;
    mat4 projectionMatrix = pushConstants.data.projectionMatrix;
    mat3 normalMatrix     = transpose(inverse(mat3(pushConstants.data.viewModelMatrix)));
    vec4 vertexPosition   = vec4(0.0);

    vertexPosition = samplePosition();


    vec4 viewPosition = viewModelMatrix * vec4(vertexPosition.xyz, 1.0);
    vec4 position     = projectionMatrix * viewPosition;
    vec3 normal       = sampleNormal();

    // Set the output variables
    outUV       = sampleTextureCoords();
    outPosition = viewPosition;
    outNormal   = normalMatrix * normal;

    gl_Position = position;

    // Flip Y for Vulkan
    gl_Position.y = -gl_Position.y;
}