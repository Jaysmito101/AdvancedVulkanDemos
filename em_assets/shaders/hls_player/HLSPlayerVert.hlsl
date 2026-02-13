#include "HLSPlayerCommon"

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};

VertexShaderOutput main(uint vertexIndex : SV_VertexID) : SV_Position {
    float3 position = positions[vertexIndex % 6];
    VertexShaderOutput output;
    output.position = float4(position, 1.0);
    output.uv = float2(position.x, position.y) * 0.5 + 0.5;
    return output;
}
