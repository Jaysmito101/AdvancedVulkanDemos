#version 450
#pragma shader_stage(fragment)

precision highp float;

#include "SubSurfaceScatteringCommon"

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D textures[];

layout(push_constant) uniform PushConstants
{
    UberPushConstantData data;
}
pushConstants;

#define KERNEL_SIZE 64
const vec3 kernel[KERNEL_SIZE] = vec3[KERNEL_SIZE](
    vec3(0.020505, 0.072441, 0.065817),
    vec3(-0.020968, 0.091786, 0.034348),
    vec3(-0.057927, 0.048430, 0.066900),
    vec3(-0.063957, -0.012858, 0.078381),
    vec3(0.036313, -0.031961, 0.091517),
    vec3(0.083516, -0.025991, 0.058978),
    vec3(0.075866, 0.028360, 0.071306),
    vec3(-0.070734, -0.067311, 0.052299),
    vec3(-0.013679, -0.105428, 0.041328),
    vec3(-0.053472, -0.072917, 0.075499),
    vec3(-0.084124, 0.034833, 0.081161),
    vec3(-0.063873, -0.020428, 0.107365),
    vec3(-0.039674, 0.036599, 0.120065),
    vec3(-0.099814, 0.042245, 0.084014),
    vec3(-0.111914, 0.081310, 0.036494),
    vec3(0.025007, -0.083887, 0.121118),
    vec3(0.110305, 0.093617, 0.059014),
    vec3(0.109145, -0.027037, 0.118697),
    vec3(-0.020388, 0.012328, 0.169525),
    vec3(0.088840, 0.121461, 0.097523),
    vec3(-0.126090, -0.008168, 0.139059),
    vec3(0.109211, 0.107784, 0.123389),
    vec3(-0.148221, -0.019370, 0.142248),
    vec3(-0.004116, -0.085759, 0.198459),
    vec3(-0.133192, 0.166222, 0.077206),
    vec3(-0.016179, 0.034820, 0.234203),
    vec3(-0.036885, -0.130449, 0.208308),
    vec3(-0.141997, -0.133450, 0.172401),
    vec3(0.270485, -0.025356, 0.017991),
    vec3(-0.091098, 0.038428, 0.267076),
    vec3(-0.073900, 0.192730, 0.214596),
    vec3(0.181130, 0.161310, 0.194910),
    vec3(0.015612, 0.239779, 0.218832),
    vec3(-0.207089, -0.266944, 0.031103),
    vec3(0.053968, 0.310594, 0.161052),
    vec3(0.069451, -0.267028, 0.245266),
    vec3(0.089854, -0.326870, 0.182008),
    vec3(-0.287144, -0.221695, 0.170425),
    vec3(0.135918, 0.247445, 0.307285),
    vec3(-0.210666, 0.246480, 0.288792),
    vec3(0.112400, -0.432507, 0.064905),
    vec3(-0.373280, -0.088699, 0.270358),
    vec3(0.270401, -0.405550, 0.012794),
    vec3(-0.009729, -0.395400, 0.316034),
    vec3(0.235231, 0.425330, 0.199491),
    vec3(-0.296074, 0.124249, 0.440305),
    vec3(-0.433912, 0.290292, 0.215893),
    vec3(-0.549596, 0.197097, 0.041976),
    vec3(-0.433416, -0.209679, 0.368409),
    vec3(-0.592543, -0.196769, 0.063333),
    vec3(0.154548, 0.303040, 0.553076),
    vec3(-0.415365, 0.407367, 0.335333),
    vec3(0.415727, 0.532217, 0.160460),
    vec3(0.497588, -0.326001, 0.400653),
    vec3(0.402050, 0.604177, 0.148311),
    vec3(0.598668, 0.439516, 0.182064),
    vec3(0.019634, 0.775790, 0.142770),
    vec3(0.518442, 0.551578, 0.298997),
    vec3(0.616150, 0.411612, 0.393859),
    vec3(0.592782, -0.395174, 0.490351),
    vec3(0.066819, -0.160032, 0.873976),
    vec3(-0.415062, -0.130118, 0.807953),
    vec3(0.612218, -0.645053, 0.318466),
    vec3(0.356930, 0.095808, 0.899105));

void main()
{
    vec2 noiseScale    = pushConstants.data.screenSizes.xy / vec2(64.0, 64.0);
    const float radius = 4.5;

    // Ignore the names, the values are correct
    mat4 projectionMatrix = pushConstants.data.projectionMatrix;

    vec2 uv     = vec2(inUV.x, inUV.y);
    vec3 origin = texture(textures[AVD_SSS_RENDER_MODE_SCENE_POSITION], uv).xyz;
    vec3 normal = normalize(texture(textures[AVD_SSS_RENDER_MODE_SCENE_NORMAL], uv).xyz * 2.0 - 1.0);
    float depth = linearizeDepth(texture(textures[AVD_SSS_RENDER_MODE_SCENE_DEPTH], uv).r);

    vec3 rVec      = texture(textures[AVD_SSS_NOISE_TEXTURE], uv * noiseScale).rgb * 2.0 - 1.0;
    rVec           = normalize(rVec);
    vec3 tangent   = normalize(rVec - normal * dot(rVec, normal));
    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;

    if (depth < origin.z + 100.0f) {

        for (uint i = 0; i < KERNEL_SIZE; i += 1) {
            vec3 sampleValue = TBN * kernel[i];
            sampleValue      = sampleValue * radius + origin;

            vec4 offset = vec4(sampleValue, 1.0);
            offset      = projectionMatrix * offset;
            offset.xy /= offset.w;
            offset.xy = offset.xy * 0.5 + 0.5; // Convert to [0, 1] range

            float sampleDepth = linearizeDepth(texture(textures[AVD_SSS_RENDER_MODE_SCENE_DEPTH], offset.xy).r);

            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(origin.z - sampleDepth));
            occlusion += (sampleDepth >= sampleValue.z + 10e-8 ? 1.0 : 0.0) * rangeCheck;
        }
    }
    occlusion = 1.0 - (occlusion / float(KERNEL_SIZE));

    outColor = vec4(occlusion);
}
