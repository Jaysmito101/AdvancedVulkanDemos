#include "MeshUtils"
#include "MathUtils"
#include "PBRUtils"

struct UberPushConstantData {
    float4x4 viewModelMatrix;
    float4x4 projectionMatrix;

    int vertexOffset;
    int vertexCount;
    int textureIndex;
    int pad1;
};

struct VertexShaderOutput {
    float4 position : SV_Position;
    float3 normal: NORMAL;
    float2 uv : TEXCOORD0;
};

const float3 cSunlightDir = float3(0.577, -0.577, 0.577);
