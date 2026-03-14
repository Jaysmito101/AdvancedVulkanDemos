#include "MeshUtils"
#include "MathUtils"

struct PushConstantData 
{
    uint vertexOffset;
    uint framebufferWidth;
    uint framebufferHeight;
    uint pad0;
};


struct VertexShaderOutput {
    float2 texCoord : TEXCOORD;
    float4 position : SV_Position;
    float3 color : COLOR;
    nointerpolation uint hasTexture : TEXCOORD1;    
};

