Texture2D fontAtlasImage : register(t0, space0);
SamplerState fontAtlasSampler : register(s0, space0);

struct PushConstantData {
    float fragmentBufferWidth;
    float fragmentBufferHeight;
    float scale;
    float opacity;
    float offsetX;
    float offsetY;
    float pxRange;
    float texSize;
    float4 color;
};

[[vk::push_constant]]
cbuffer PushConstants {
    PushConstantData data;
};

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(float2 texCoord) {
    float2 unitRange = float2(data.pxRange / data.texSize, data.pxRange / data.texSize);
    float2 screenTexSize = float2(1.0, 1.0) / fwidth(texCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float4 main(float2 fragTexCoord : TEXCOORD0) : SV_Target {
    float2 txCoord = float2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    float3 msd = fontAtlasImage.Sample(fontAtlasSampler, txCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange(txCoord) * (sd - 0.5);
    float alpha = smoothstep(0.0, 1.0, screenPxDistance + 0.5);
    return float4(data.color.rgb, data.color.a * data.opacity * alpha);
}
