#include "EyeballsSceneCommon"

float4 main(VertexTransfer inVertex) : SV_Target
{
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float3 normal = normalize(inVertex.normal);
    float diffuse = max(dot(normal, lightDir), 0.0) + 0.1;
    float3 albedo = float3(0.6, 0.8, 1.0);
    float3 color = albedo * diffuse; 
    return float4(color, 1.0);
}