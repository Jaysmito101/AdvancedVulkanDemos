#include "DrawListCommon"

struct VertexShaderOutput {
    float2 texCoord : TEXCOORD;
    float4 position : SV_Position;
};

[[vk::push_constant]]
cbuffer PushConstants {
    PushConstantData data;
};



VertexShaderOutput main(uint vertexId : SV_VertexID) {
    float2 positions[3] = {
        float2(0.0, -0.5),
        float2(0.5, 0.5),
        float2(-0.5, 0.5)
    };

    float2 texCoords[3] = {
        float2(0.5, 0.0),
        float2(1.0, 1.0),
        float2(0.0, 1.0)
    };

    VertexShaderOutput output;
    output.texCoord = texCoords[vertexId];
    output.position = float4(positions[vertexId], 0.0, 1.0);
    return output;
}