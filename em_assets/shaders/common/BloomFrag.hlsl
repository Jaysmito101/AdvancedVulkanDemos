// from: https://github.com/Unity-Technologies/Graphics/blob/master/com.unity.postprocessing/PostProcessing/Shaders/Builtins/Bloom.shader

#include "MathUtils"

Texture2D customTexture0 : register(t0, space0);
SamplerState sampler0 : register(s0, space0);

Texture2D customTexture1 : register(t0, space1);
SamplerState sampler1 : register(s0, space1);

#define AVD_BLOOM_PASS_TYPE_PREFILTER 0
#define AVD_BLOOM_PASS_TYPE_DOWNSAMPLE 1
#define AVD_BLOOM_PASS_TYPE_UPSAMPLE 2
#define AVD_BLOOM_PASS_TYPE_COMPOSITE 3
#define AVD_BLOOM_PASS_TYPE_DOWNSAMPLE_PREFILTER 4

#define AVD_BLOOM_PREFILTER_TYPE_NONE 0
#define AVD_BLOOM_PREFILTER_TYPE_THRESHOLD 1
#define AVD_BLOOM_PREFILTER_TYPE_SOFTKNEE 2

#define AVD_BLOOM_TONEMAPPING_TYPE_NONE 0
#define AVD_BLOOM_TONEMAPPING_TYPE_ACES 1
#define AVD_BLOOM_TONEMAPPING_TYPE_FILMIC 2

struct PushConstantData
{
    float srcWidth;
    float srcHeight;
    float targetWidth;
    float targetHeight;

    int bloomPrefilterType;
    int type;
    float bloomPrefilterThreshold;
    float softKnee;

    float bloomAmount;
    int lowerQuality;
    int tonemappingType;
    int applyGamma;
};

[[vk::push_constant]] cbuffer PushConstants
{
    PushConstantData data;
}

float luminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// -------------- Utility functions -------------- //

// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
float3 quadraticThreshold(float3 color, float threshold, float3 curve)
{
    // Pixel brightness
    float br = luminance(color);

    // Under-threshold part
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;

    // Combine and apply the brightness response curve
    color *= max(rq, br - threshold) / max(br, 1e-4);

    return color;
}

float3 aces(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

float3 filmic(float3 z)
{
    float a = 0.1;
    float b = 1.1;
    float c = 0.5;
    float d = 0.02;
    float e = 1.3;
    float f = 4.8;
    float g = 0.3;
    float h = 2.0;
    float i = 0.2;
    float j = 0.6;
    float k = 1.3;
    float l = 2.5;

    z *= 20.0;

    z = h * (c + pow(a * z, float3(b, b, b)) - d * (sin(e * z) - j) / ((k * z - f) * (k * z - f) + g));
    z = pow(z, float3(l, l, l));
    z = i * log(z);

    return clamp(z, 0.0, 1.0);
}

float3 prefilter(float3 color)
{
    if (data.bloomPrefilterType == AVD_BLOOM_PREFILTER_TYPE_THRESHOLD)
    {
        float l = luminance(color);
        float threshold = data.bloomPrefilterThreshold;
        if (l < threshold)
        {
            return float3(0.0, 0.0, 0.0);
        }
        else
        {
            return color;
        }
    }
    else if (data.bloomPrefilterType == AVD_BLOOM_PREFILTER_TYPE_SOFTKNEE)
    {
        float threshold = data.bloomPrefilterThreshold;
        float knee = data.softKnee;
        float3 curve = float3(threshold - knee, knee * 2.0, 0.25 / knee);
        return quadraticThreshold(color, threshold, curve);
    }
    else
    {
        return color;
    }
}

float4 downsample13Tap(Texture2D textureToSample, SamplerState samplerToUse, float2 uv)
{
    float2 texelSize = float2(1.0 / data.srcWidth, 1.0 / data.srcHeight);

    float4 A = textureToSample.Sample(samplerToUse, uv + texelSize * float2(-2, -2));
    float4 B = textureToSample.Sample(samplerToUse, uv + texelSize * float2(0, -2));
    float4 C = textureToSample.Sample(samplerToUse, uv + texelSize * float2(2, -2));
    float4 D = textureToSample.Sample(samplerToUse, uv + texelSize * float2(-1, -1));
    float4 E = textureToSample.Sample(samplerToUse, uv + texelSize * float2(1, -1));
    float4 F = textureToSample.Sample(samplerToUse, uv + texelSize * float2(-2, 0));
    float4 G = textureToSample.Sample(samplerToUse, uv);
    float4 H = textureToSample.Sample(samplerToUse, uv + texelSize * float2(2, 0));
    float4 I = textureToSample.Sample(samplerToUse, uv + texelSize * float2(-1, 1));
    float4 J = textureToSample.Sample(samplerToUse, uv + texelSize * float2(1, 1));
    float4 K = textureToSample.Sample(samplerToUse, uv + texelSize * float2(-2, 2));
    float4 L = textureToSample.Sample(samplerToUse, uv + texelSize * float2(0, 2));
    float4 M = textureToSample.Sample(samplerToUse, uv + texelSize * float2(2, 2));

    float2 div = (1.0 / 4.0) * float2(0.5, 0.125);

    float4 o = (D + E + I + J) * div.x;
    o += (A + B + G + F) * div.y;
    o += (B + C + H + G) * div.y;
    o += (F + G + L + K) * div.y;
    o += (G + H + M + L) * div.y;

    return float4(o.rgb, G.a);
}

float4 downsample4Tap(Texture2D textureToSample, SamplerState samplerToUse, float2 uv)
{
    float2 texelSize = float2(1.0 / data.srcWidth, 1.0 / data.srcHeight);

    float4 d = texelSize.xyxy * float4(-1.0, -1.0, 1.0, 1.0);

    float4 s = float4(0.0, 0.0, 0.0, 0.0);
    s += textureToSample.Sample(samplerToUse, uv + d.xy);
    s += textureToSample.Sample(samplerToUse, uv + d.zy);
    s += textureToSample.Sample(samplerToUse, uv + d.xw);
    s += textureToSample.Sample(samplerToUse, uv + d.zw);

    return s * 0.25;
}

float4 downsample(Texture2D textureToSample, SamplerState samplerToUse, float2 uv)
{
    if (data.lowerQuality == 1)
    {
        return downsample4Tap(textureToSample, samplerToUse, uv);
    }
    else
    {
        return downsample13Tap(textureToSample, samplerToUse, uv);
    }
}

float4 upsampleTent(Texture2D textureToSample, SamplerState samplerToUse, float2 uv)
{
    float2 texelSize = float2(1.0 / data.targetWidth, 1.0 / data.targetHeight);

    float4 centralValue = textureToSample.Sample(samplerToUse, uv);
    float4 d = texelSize.xyxy * float4(1, 1, -1, 0);
    float4 s;
    s = textureToSample.Sample(samplerToUse, uv - d.xy);
    s += textureToSample.Sample(samplerToUse, uv - d.wy) * 2.0;
    s += textureToSample.Sample(samplerToUse, uv - d.zy);
    s += textureToSample.Sample(samplerToUse, uv + d.zw) * 2.0;
    s += centralValue * 4.0;
    s += textureToSample.Sample(samplerToUse, uv + d.xw) * 2.0;
    s += textureToSample.Sample(samplerToUse, uv + d.zy);
    s += textureToSample.Sample(samplerToUse, uv + d.wy) * 2.0;
    s += textureToSample.Sample(samplerToUse, uv + d.xy);

    float4 o = s / 16.0;
    return float4(o.rgb, centralValue.a);
}

float4 upsampleBox(Texture2D textureToSample, SamplerState samplerToUse, float2 uv)
{
    float2 texelSize = float2(1.0 / data.targetWidth, 1.0 / data.targetHeight);

    float4 d = texelSize.xyxy * float4(-1.0, -1.0, 1.0, 1.0);

    float4 s = float4(0.0, 0.0, 0.0, 0.0);
    s += textureToSample.Sample(samplerToUse, uv + d.xy);
    s += textureToSample.Sample(samplerToUse, uv + d.zy);
    s += textureToSample.Sample(samplerToUse, uv + d.xw);
    s += textureToSample.Sample(samplerToUse, uv + d.zw);

    return s * (1.0 / 4.0);
}

float4 upsample(Texture2D textureToSample, SamplerState samplerToUse, float2 uv)
{
    if (data.lowerQuality == 1)
    {
        return upsampleBox(textureToSample, samplerToUse, uv);
    }
    else
    {
        return upsampleTent(textureToSample, samplerToUse, uv);
    }
}

// -------------- Pass functions -------------- //

float4 prefilterPass(float2 uv)
{
    float4 color = customTexture0.Sample(sampler0, uv);
    float3 prefilteredColor = prefilter(color.rgb);
    float4 bloomColor = float4(prefilteredColor, color.a);
    return bloomColor;
}

float4 downsamplePrefilterPass(float2 uv)
{
    float4 color = downsample(customTexture0, sampler0, uv);
    float3 prefilteredColor = prefilter(color.rgb);
    float4 bloomColor = float4(prefilteredColor, color.a);
    return bloomColor;
}

float4 downsamplePass(float2 uv)
{
    float4 color = downsample(customTexture0, sampler0, uv);
    return color;
}

float4 upsamplePass(float2 uv)
{
    float4 originalColor = customTexture1.Sample(sampler1, uv);
    float4 color = upsample(customTexture0, sampler0, uv);
    float4 combined = color + originalColor;
    return float4(combined.rgb, originalColor.a);
}

float4 compositePass(float2 uv)
{
    float4 bloomColor = upsample(customTexture0, sampler0, uv);
    float4 sceneColor = customTexture1.Sample(sampler1, uv);
    float bloomAmount = data.bloomAmount;
    float3 result = bloomColor.rgb * bloomAmount + sceneColor.rgb;
    if (data.tonemappingType == AVD_BLOOM_TONEMAPPING_TYPE_ACES)
    {
        result = aces(result);
    }
    else if (data.tonemappingType == AVD_BLOOM_TONEMAPPING_TYPE_FILMIC)
    {
        result = filmic(result);
    }
    if (data.applyGamma == 1)
    {
        result = pow(result, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    }
    return float4(result.rgb, sceneColor.a);
}

float4 main(float2 inUV : TEXCOORD0) : SV_Target
{
    float4 outColor = float4(0.0, 0.0, 0.0, 0.0);
    if (data.type == AVD_BLOOM_PASS_TYPE_PREFILTER)
    {
        outColor = prefilterPass(inUV);
    }
    else if (data.type == AVD_BLOOM_PASS_TYPE_DOWNSAMPLE)
    {
        outColor = downsamplePass(inUV);
    }
    else if (data.type == AVD_BLOOM_PASS_TYPE_UPSAMPLE)
    {
        outColor = upsamplePass(inUV);
    }
    else if (data.type == AVD_BLOOM_PASS_TYPE_COMPOSITE)
    {
        outColor = compositePass(inUV);
    }
    else if (data.type == AVD_BLOOM_PASS_TYPE_DOWNSAMPLE_PREFILTER)
    {
        outColor = downsamplePrefilterPass(inUV);
    }
    return outColor;
}