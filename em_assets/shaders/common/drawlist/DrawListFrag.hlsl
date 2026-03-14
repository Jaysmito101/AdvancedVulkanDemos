#include "DrawListCommon"


[[vk::binding(0, 1)]]
SAMPLER2D(tex, 0, 1);

[[vk::push_constant]]
cbuffer PushConstants {
    PushConstantData data;
};



float screenPxRange(float2 texCoord) {
    uint2 texSize = TEXTURE_SIZE(tex);
    float2 unitRange = float2(data.fontPxRange / float(texSize.x), data.fontPxRange / float(texSize.y));
    float2 screenTexSize = float2(1.0, 1.0) / fwidth(texCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float4 fontGetColor(float2 texCoord) {
    texCoord.y = 1.0 - texCoord.y;
    float3 msd = SAMPLE_TEXTURE(tex, texCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange(texCoord) * (sd - 0.5);
    float alpha = smoothstep(0.0, 1.0, screenPxDistance + 0.5);
    return float4(1.0, 1.0, 1.0, alpha);
}


float4 main(VertexShaderOutput input) : SV_Target {
    
    float4 color = float4(1.0);
    if (input.hasTexture == 1) {
        color = SAMPLE_TEXTURE(tex, input.texCoord);
    } else if (input.hasTexture == 2) {
        color = fontGetColor(input.texCoord);
    }
    color *= float4(input.color, 1.0);

    return color;
}
