#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require


#include "MeshUtils"
#include "MathUtils"
#include "PBRUtils"

struct UberPushConstantData {
    float4x4 viewModelMatrix;
    float4x4 projectionMatrix;

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