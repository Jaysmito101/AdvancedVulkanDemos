#include "DeccerCubeCommon"

[[vk::binding(0, 0)]]
StructuredBuffer<ModelVertex> vertices : register(t0, space0);

[[vk::binding(1, 0)]]
StructuredBuffer<uint> indices : register(t1, space0);

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};

uint getVertexIndex(uint vertexIndex) {
    return indices[vertexIndex + data.vertexOffset];
}

float4 samplePosition(uint vertexIndex)
{
    uint index = getVertexIndex(vertexIndex);
    return float4(vertices[index].vx, vertices[index].vy, vertices[index].vz, 1.0);
}

float2 sampleTextureCoords(uint vertexIndex)
{
    uint index = getVertexIndex(vertexIndex);
    return float2(vertices[index].tu, vertices[index].tv);
}

float3 sampleNormal(uint vertexIndex)
{
    uint index = getVertexIndex(vertexIndex);
    float3 normal;
    float4 tangent;
    unpackTBN(vertices[index].np, uint(vertices[index].tp), normal, tangent);
    return normalize(normal);
}

VertexShaderOutput main(uint vertexIndex : SV_VertexID)
{
    VertexShaderOutput output;

    float4x4 modelMatrix      = data.modelMatrix;
    float4x4 viewMatrix       = data.viewMatrix;
    float4x4 viewModelMatrix  = mul(viewMatrix, modelMatrix);
    float4x4 projectionMatrix = data.projectionMatrix;
    float3x3 normalMatrix     = transpose(inverse(mat3(data.modelMatrix)));
    float4 vertexPosition     = float4(0.0);

    vertexPosition = samplePosition(vertexIndex);


    float4 viewPosition = mul(viewModelMatrix, float4(vertexPosition.xyz, 1.0));
    float4 position     = mul(projectionMatrix, viewPosition);
    float3 normal       = sampleNormal(vertexIndex);

    // Set the output variables
    output.uv       = sampleTextureCoords(vertexIndex);
    output.position = viewPosition;
    output.normal = mul(normalMatrix, normal);
    output.targetPosition = position;
    output.targetPosition.y *= -1.0;

    return output;
}
