#include "RealisticHeadCommon"

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};

[[vk::binding(0, 0)]]
StructuredBuffer<ModelVertex> vertices : register(t0, space0);

[[vk::binding(1, 0)]]
StructuredBuffer<uint> indices : register(t1, space0);

uint getVertexIndex(uint vertexID) {
    return indices[vertexID + data.vertexOffset];
}

float3 sampleNormal(uint vertexID)
{
    uint index = getVertexIndex(vertexID);
    float3 normal;
    float4 tangent;
    unpackTBN(vertices[index].np, uint(vertices[index].tp), normal, tangent);
    return normalize(normal);
}

float4 samplePosition(uint vertexID)
{
uint index = getVertexIndex(vertexID);
    return float4(vertices[index].vx, vertices[index].vy, vertices[index].vz, 1.0);
}

float2 sampleTexCoord(uint vertexID)
{
    uint index = getVertexIndex(vertexID);
    return float2(vertices[index].tu, vertices[index].tv);
}

VertexShaderOutput main(uint vertexID : SV_VertexID) {
    VertexShaderOutput output;

    float4x4 viewModelMatrix = data.viewModelMatrix;
    ctionMatrix, mul(viewModelMatrix, position));

    float3 normal = sampleNormal(vertexID);
    float2 texCoord = sampleTexCoord(vertexID);

    output.position = targetPosition;
    output.normal = normal;
    output.uv = texCoord; 
    output.position.y *= -1.0;


    return output;
}
