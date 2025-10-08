#include "HLSPlayerCommon"

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};

static const float3 positions[6] = {
    float3(-1.0, -1.0, 0.0),
    float3( 1.0, -1.0, 0.0),
    float3(-1.0,  1.0, 0.0),
    float3( 1.0, -1.0, 0.0),
    float3( 1.0,  1.0, 0.0),
    float3(-1.0,  1.0, 0.0)
};

float4 main(uint vertexIndex : SV_VertexID) : SV_Position {
    float3 position = positions[vertexIndex];
    return float4(position, 1.0);
}
