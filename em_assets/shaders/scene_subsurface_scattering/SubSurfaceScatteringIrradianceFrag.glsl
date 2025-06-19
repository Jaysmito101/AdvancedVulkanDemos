#version 450
#pragma shader_stage(fragment)

precision highp float;

#include "SubSurfaceScatteringCommon"

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2D textures[];

layout(location = 0) out vec4 outDiffusedIrradiance;

layout(push_constant) uniform PushConstants
{
    LightPushConstantData data;
}
pushConstants;

const int NUM_SAMPLES = 7;
float sss_offsets[NUM_SAMPLES] = float[](0.0, 0.1, 0.25, 0.45, 0.65, 0.85, 1.0);
float sss_weights[NUM_SAMPLES] = float[](0.2, 0.18, 0.15, 0.12, 0.08, 0.05, 0.02);


void main()
{
    vec2 texelSize = vec2(1.0 / textureSize(textures[AVD_SSS_RENDER_MODE_SCENE_DIFFUSE], 0));
    vec2 uv = inUV;

    const float sssWidth = pushConstants.data.screenSpaceIrradianceScale;

    float depth = linearizeDepth(texture(textures[AVD_SSS_RENDER_MODE_SCENE_DEPTH], uv).r);
    float depthScale = sssWidth / (depth + 0.0001);

    vec3 irradiance = vec3(0.0);
    
    // Ideally we should do this in two passes as they are seperable kernels,
    // but for simplicity we will do it in one pass here.
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        for (int j = 0; j < NUM_SAMPLES; ++j) {
            vec2 offset = vec2(sss_offsets[i], sss_offsets[j]) * texelSize * depthScale;
            float weight = sss_weights[i] * sss_weights[j];
            irradiance += texture(textures[AVD_SSS_RENDER_MODE_SCENE_DIFFUSE], uv + offset).rgb * weight;
            irradiance += texture(textures[AVD_SSS_RENDER_MODE_SCENE_DIFFUSE], uv - offset).rgb * weight;
        }
    }

    outDiffusedIrradiance = vec4(irradiance, 1.0);
}