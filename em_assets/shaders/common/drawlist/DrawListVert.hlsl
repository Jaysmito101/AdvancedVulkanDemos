#include "DrawListCommon"

[[vk::binding(0, 0)]]
StructuredBuffer<ModelVertex> vertices : register(t0, space0);

[[vk::push_constant]]
cbuffer PushConstants {
    PushConstantData data;
};


float3 sampleColor(uint vertexIndex)
{
    float3 normal;
    float4 tangent;
    unpackTBN(vertices[vertexIndex].np, uint(vertices[vertexIndex].tp), normal, tangent);
    return normal;
}


VertexShaderOutput main(uint vertexId : SV_VertexID) {
    uint vertexIndex = vertexId + data.vertexOffset;

    float2 position = float2(vertices[vertexIndex].vx, vertices[vertexIndex].vy) / float2(data.framebufferWidth, data.framebufferHeight);
    position = position * 2.0 - 1.0;

    float2 uv = float2(vertices[vertexIndex].tu, vertices[vertexIndex].tv);
    float3 color = sampleColor(vertexIndex);

    VertexShaderOutput output;
    output.texCoord = uv;
    output.position = float4(position, 0.0, 1.0);
    output.color = color;
    output.hasTexture = uint(float(vertices[vertexIndex].vz)); 
    return output;
}