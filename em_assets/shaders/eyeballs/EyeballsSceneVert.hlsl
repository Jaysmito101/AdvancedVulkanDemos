
#include "EyeballsSceneCommon"


float4 samplePosition(uint index)
{
    index = eyeballIndices[index];
    return float4(eyeballVertices[index].vx, eyeballVertices[index].vy, eyeballVertices[index].vz, 1.0);
}


VertexTransfer main(uint vertexIndex : SV_VertexID)
{
    uint indexOffset = vertexIndex + (uint)data.indexOffset;
    float4x4 projectionMatrix = data.projectionMatrix;
    float4x4 viewModelMatrix = data.viewModelMatrix;


    float4 position = samplePosition(indexOffset);
    float4 viewPos = mul(viewModelMatrix, position);
    float4 clipPosition = mul(projectionMatrix, viewPos);
    clipPosition.y *= -1.0;

    VertexTransfer output;
    output.position = clipPosition;
    output.normal = normalize(position.xyz);
    output.viewPos = viewPos.xyz;
    
    return output;
}