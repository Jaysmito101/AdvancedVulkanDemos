#include "MeshUtils"
#include "MathUtils"
#include "PBRUtils"

struct UberPushConstantData {
    float4x4 modelMatrix;
    float4x4 projectionMatrix;
    float4x4 viewMatrix;

    uint vertexOffset;
    uint vertexCount;
    uint textureIndex;
    uint pad1;
};

struct VertexShaderOutput {
    float2 uv : TEXCOORD0;
    float3 normal: NORMAL;
    float4 position : POSITION;
    float4 targetPosition : SV_Position;
};
