#include "HLSPlayerCommon"

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};
 
[[vk::binding(1, 1)]]
SAMPLER2D_TAB(textures, 1);

float sdRoundedBox(float2 p, float2 b, float r) {
    float2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

float4 main(VertexShaderOutput input) : SV_Target {
    float2 uv = input.uv * 2.0 - 1.0;
    uv *= 1.1 + 0.5;
    uv = uv * 0.5 + 0.5;

    float2 cellCoord = uv  * float2(3.0, 2.0);
    int2 cellIndex = int2(floor(cellCoord));
    if (cellIndex.x < 0 || cellIndex.y < 0 || cellIndex.x > 2 || cellIndex.y > 1) {
        return float4(0.0);
    }

    int quadIndex = cellIndex.y * 3 + cellIndex.x;
    
    float2 localUV = frac(cellCoord);
    float2 centeredUV = localUV * 2.0 - 1.0;
    
    float padding = 0.2;
    float cornerRadius = 0.05;
    float borderWidth = 0.02;
    
    float dist = sdRoundedBox(centeredUV, float2(1.0 - padding, 1.0 - padding), cornerRadius);
    
    float3 uvColor = float3(localUV, 0.5);
    
    bool isSelected = (quadIndex == int(1));
    float3 borderColor = isSelected ? float3(1.0, 0.8, 0.0) : float3(0.3, 0.3, 0.3);
    float actualBorderWidth = isSelected ? borderWidth * 2.0 : borderWidth;
    
    float3 finalColor = uvColor; 
    if (dist > -actualBorderWidth && dist < 0.0) {  
        finalColor = borderColor;
    }
    else if (dist >= 0.0) { 
        finalColor = float3(0.1, 0.1, 0.1);
    }
    
    return float4(finalColor, 1.0); 
}
