#version 450

// from: https://github.com/Unity-Technologies/Graphics/blob/master/com.unity.postprocessing/PostProcessing/Shaders/Builtins/Bloom.shader

#pragma shader_stage(fragment)

layout(set = 0, binding = 0) uniform sampler2D customTexture0;
layout(set = 1, binding = 0) uniform sampler2D customTexture1;
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

#define AVD_BLOOM_PASS_TYPE_PREFILTER               0
#define AVD_BLOOM_PASS_TYPE_DOWNSAMPLE              1
#define AVD_BLOOM_PASS_TYPE_UPSAMPLE                2
#define AVD_BLOOM_PASS_TYPE_COMPOSITE               3
#define AVD_BLOOM_PASS_TYPE_DOWNSAMPLE_PREFILTER    4


#define AVD_BLOOM_PREFILTER_TYPE_NONE      0
#define AVD_BLOOM_PREFILTER_TYPE_THRESHOLD 1
#define AVD_BLOOM_PREFILTER_TYPE_SOFTKNEE  2

#define AVD_BLOOM_TONEMAPPING_TYPE_NONE    0
#define AVD_BLOOM_TONEMAPPING_TYPE_ACES    1
#define AVD_BLOOM_TONEMAPPING_TYPE_FILMIC  2

struct PushConstantData {
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

layout(push_constant) uniform PushConstants
{
    PushConstantData data;
}
pushConstants;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// -------------- Utility functions -------------- //

// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
vec3 quadraticThreshold(vec3 color, float threshold, vec3 curve)
{
    // Pixel brightness
    float br = luminance(color);

    // Under-threshold part
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq       = curve.z * rq * rq;

    // Combine and apply the brightness response curve
    color *= max(rq, br - threshold) / max(br, 1e-4);

    return color;
}


vec3 aces(vec3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

vec3 filmic(vec3 z)
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

    z = h * (c + pow(a * z, vec3(b)) - d * (sin(e * z) - j) / ((k * z - f) * (k * z - f) + g));
    z = pow(z, vec3(l));
    z = i * log(z);

    return clamp(z, 0.0, 1.0);
}

vec3 prefilter(vec3 color)
{
    if (pushConstants.data.bloomPrefilterType == AVD_BLOOM_PREFILTER_TYPE_THRESHOLD) {
        float luminance = luminance(color);
        float threshold = pushConstants.data.bloomPrefilterThreshold;
        if (luminance < threshold) {
            return vec3(0.0, 0.0, 0.0);
        } else {
            return color;
        }
    } else if (pushConstants.data.bloomPrefilterType == AVD_BLOOM_PREFILTER_TYPE_SOFTKNEE) {
        float threshold = pushConstants.data.bloomPrefilterThreshold;
        float knee      = pushConstants.data.softKnee;
        vec3 curve      = vec3(threshold - knee, knee * 2.0, 0.25 / knee);
        return quadraticThreshold(color, threshold, curve);
    } else {
        return color;
    }
}

vec4 downsample13Tap(sampler2D textureToSample, vec2 uv) {
    vec2 texelSize = vec2(1.0 / pushConstants.data.srcWidth, 1.0 / pushConstants.data.srcHeight);

    vec4 A = texture(textureToSample, uv + texelSize * vec2(-2, -2));
    vec4 B = texture(textureToSample, uv + texelSize * vec2(0, -2));
    vec4 C = texture(textureToSample, uv + texelSize * vec2(2, -2));
    vec4 D = texture(textureToSample, uv + texelSize * vec2(-1, -1));
    vec4 E = texture(textureToSample, uv + texelSize * vec2(1, -1));
    vec4 F = texture(textureToSample, uv + texelSize * vec2(-2, 0));
    vec4 G = texture(textureToSample, uv);
    vec4 H = texture(textureToSample, uv + texelSize * vec2(2, 0));
    vec4 I = texture(textureToSample, uv + texelSize * vec2(-1, 1));
    vec4 J = texture(textureToSample, uv + texelSize * vec2(1, 1));
    vec4 K = texture(textureToSample, uv + texelSize * vec2(-2, 2));
    vec4 L = texture(textureToSample, uv + texelSize * vec2(0, 2));
    vec4 M = texture(textureToSample, uv + texelSize * vec2(2, 2));

    vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);

    vec4 o = (D + E + I + J) * div.x;
    o += (A + B + G + F) * div.y;
    o += (B + C + H + G) * div.y;
    o += (F + G + L + K) * div.y;
    o += (G + H + M + L) * div.y;

    return vec4(o.rgb, G.a);
}

vec4 downscample4Tap(sampler2D textureToSample, vec2 uv) {
    vec2 texelSize = vec2(1.0 / pushConstants.data.srcWidth, 1.0 / pushConstants.data.srcHeight);

    vec4 d = texelSize.xyxy * vec4(-1.0, -1.0, 1.0, 1.0);

    vec4 s = vec4(0.0);
    s += texture(textureToSample, uv + d.xy);
    s += texture(textureToSample, uv + d.zy);
    s += texture(textureToSample, uv + d.xw);
    s += texture(textureToSample, uv + d.zw);
    
    return s * 0.25;
}

vec4 downsample(sampler2D textureToSample, vec2 uv) {
    if (pushConstants.data.lowerQuality == 1) {
        return downscample4Tap(textureToSample, uv);
    } else {
        return downsample13Tap(textureToSample, uv);
    }
}


vec4 upsampleTent(sampler2D textureToSample, vec2 uv) {
    vec2 texelSize = vec2(1.0 / pushConstants.data.targetWidth, 1.0 / pushConstants.data.targetHeight);

    vec4 centralValue = texture(textureToSample, uv);
    vec4 d = texelSize.xyxy * vec4(1, 1, -1, 0);
    vec4 s;
    s = texture(textureToSample, uv - d.xy);
    s += texture(textureToSample, uv - d.wy) * 2.0;
    s += texture(textureToSample, uv - d.zy);
    s += texture(textureToSample, uv + d.zw) * 2.0;
    s += centralValue * 4.0;
    s += texture(textureToSample, uv + d.xw) * 2.0;
    s += texture(textureToSample, uv + d.zy);
    s += texture(textureToSample, uv + d.wy) * 2.0;
    s += texture(textureToSample, uv + d.xy);

    vec4 o = s / 16.0;
    return vec4(o.rgb, centralValue.a);
}

vec4 upsampleBox(sampler2D textureToSample, vec2 uv) {
    vec2 texelSize = vec2(1.0 / pushConstants.data.targetWidth, 1.0 / pushConstants.data.targetHeight);

    vec4 d = texelSize.xyxy * vec4(-1.0, -1.0, 1.0, 1.0);

    vec4 s = vec4(0.0);
    s += texture(textureToSample, uv + d.xy);
    s += texture(textureToSample, uv + d.zy);
    s += texture(textureToSample, uv + d.xw);
    s += texture(textureToSample, uv + d.zw);    

    return s * (1.0 / 4.0);
}

vec4 upsample(sampler2D textureToSample, vec2 uv) {
    if (pushConstants.data.lowerQuality == 1) {
        return upsampleBox(textureToSample, uv);
    } else {
        return upsampleTent(textureToSample, uv);
    }
}

// -------------- Utility functions -------------- //

vec4 prefilterPass()
{
    vec4 color = texture(customTexture0, inUV);
    vec3 prefilteredColor = prefilter(color.rgb);
    vec4 bloomColor = vec4(prefilteredColor, color.a);
    return bloomColor;
}

vec4 downsamplePrefilterPass()
{
    vec4 color = downsample(customTexture0, inUV);
    vec3 prefilteredColor = prefilter(color.rgb);
    vec4 bloomColor = vec4(prefilteredColor, color.a);
    return bloomColor;
}

// to a 13 tap downsample here
vec4 downsamplePass()
{
    vec4 color = downsample(customTexture0, inUV);
    return color;
}

// upsample tent
vec4 upsamplePass()
{
    vec4 originalColor = texture(customTexture1, inUV);
    vec4 color = upsample(customTexture0, inUV);
    vec4 combined = color + originalColor;
    return vec4(combined.rgb, originalColor.a);
}

vec4 compositePass()
{
    vec4 bloomColor = upsample(customTexture0, inUV);
    vec4 sceneColor = texture(customTexture1, inUV);
    float bloomAmount = pushConstants.data.bloomAmount;
    vec3 result = bloomColor.rgb * bloomAmount + sceneColor.rgb;
    if (pushConstants.data.tonemappingType == AVD_BLOOM_TONEMAPPING_TYPE_ACES) {
        result = aces(result);
    } else if (pushConstants.data.tonemappingType == AVD_BLOOM_TONEMAPPING_TYPE_FILMIC) {
        result = filmic(result);
    }
    if (pushConstants.data.applyGamma == 1) {
        result = pow(result, vec3(1.0 / 2.2));
    }
    outColor.rgb = result;
    outColor.a   = sceneColor.a;
    return outColor;
}

void main()
{
    outColor = vec4(0.0, 0.0, 0.0, 0.0);
    if (pushConstants.data.type == AVD_BLOOM_PASS_TYPE_PREFILTER) {
        outColor = prefilterPass();
    } else if (pushConstants.data.type == AVD_BLOOM_PASS_TYPE_DOWNSAMPLE) {
        outColor = downsamplePass();
    } else if (pushConstants.data.type == AVD_BLOOM_PASS_TYPE_UPSAMPLE) {
        outColor = upsamplePass();
    } else if (pushConstants.data.type == AVD_BLOOM_PASS_TYPE_COMPOSITE) {
        outColor = compositePass();
    } else if (pushConstants.data.type == AVD_BLOOM_PASS_TYPE_DOWNSAMPLE_PREFILTER) {
        outColor = downsamplePrefilterPass();
    }
}