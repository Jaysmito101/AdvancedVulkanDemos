#include "MathUtils"

struct UberPushConstantData {
    uint activeSources;
    uint vertexCount;
    int textureIndices;
    uint pad1;
    float4 cameraPosition;
    float4 cameraDirection;
};

static const float3 positions[6] = {
    float3(-1.0, -1.0, 0.0),
    float3( 1.0, -1.0, 0.0),
    float3(-1.0,  1.0, 0.0),
    float3( 1.0, -1.0, 0.0),
    float3( 1.0,  1.0, 0.0),
    float3(-1.0,  1.0, 0.0)
};

struct VertexShaderOutput {
    float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};