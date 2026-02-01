#include "HLSPlayerCommon"

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};
 
[[vk::binding(1, 0)]]
SAMPLER2D_TAB(textures, 1);

float sdRoundedBox(float2 p, float2 b, float r) {
    float2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

float4 main(VertexShaderOutput input) : SV_Target {
    float aspect = 16.0 / 9.0;
    float2 uv = input.uv;
    uv.x *= aspect;

    float quadHeight = 0.38;
    float quadWidth = quadHeight * aspect;
    float gap = 0.2;
    float cornerRadius = 0.02;
    float dotAreaHeight = 0.015;
    float titleOffset = 0.06;

    float2 quadSize = float2(quadWidth, quadHeight);
    float2 gridSize = float2(2.0 * quadWidth + gap, 2.0 * (quadHeight + dotAreaHeight) + gap);
    float2 gridCenter = float2(aspect * 0.5, 0.5 + titleOffset);
    float2 gridStart = gridCenter - gridSize * 0.5;

    float2 cellSize = float2(quadWidth, quadHeight + dotAreaHeight);
    float2 cellStride = cellSize + float2(gap, gap) * 0.5;

    float2 relPos = uv - gridStart;
    int2 cellIndex = int2(floor(relPos / cellStride));
    if (any(cellIndex < 0) || any(cellIndex > 1)) return float4(0.0, 0.0, 0.0, 1.0);

    float2 localPos = relPos - float2(cellIndex) * cellStride;
    if (localPos.x > cellSize.x || localPos.y > cellSize.y) return float4(0.0, 0.0, 0.0, 1.0);

    int quadIndex = cellIndex.y * 2 + cellIndex.x;
    bool isSourceActive = (data.activeSources & (1u << (quadIndex * 8))) != 0;
    float fadeFactor = isSourceActive ? 1.0 : 0.3;

    if (localPos.y < dotAreaHeight) {
        float dotWidth = dotAreaHeight;
        float totalDotsWidth = dotWidth * 7.0;
        float dotsStartX = (cellSize.x - totalDotsWidth) * 0.5;
        float dotLocalX = localPos.x - dotsStartX;
        if (dotLocalX < 0.0 || dotLocalX > totalDotsWidth) return float4(0.0, 0.0, 0.0, 1.0);
        
        int dotIndex = int(floor(dotLocalX / dotWidth));
        float2 dotUV = float2(frac(dotLocalX / dotWidth), localPos.y / dotAreaHeight);
        if (length(dotUV - 0.5) < 0.4) {
            bool isDotActive = (data.activeSources & (1u << (quadIndex * 8 + dotIndex + 1))) != 0;
            float3 activeColor = (dotIndex == 0) ? float3(0.0, 0.5, 1.0) : float3(0.0, 1.0, 0.0);
            float3 dotColor = isDotActive ? activeColor : float3(0.3);
            return float4(dotColor * fadeFactor, 1.0);
        }
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    float2 quadLocalPos = float2(localPos.x, localPos.y - dotAreaHeight);
    float2 quadCenterPos = quadSize * 0.5;
    float2 centeredPos = quadLocalPos - quadCenterPos;

    float dist = sdRoundedBox(centeredPos, quadCenterPos - cornerRadius, cornerRadius);
    if (dist > 0.0) return float4(0.0, 0.0, 0.0, 1.0);

    float2 quadUV = quadLocalPos / quadSize;
    float4 texColor = SAMPLE_TEXTURE_TAB(textures, quadUV, quadIndex);
    return float4(texColor.rgb * fadeFactor, 1.0);
}
