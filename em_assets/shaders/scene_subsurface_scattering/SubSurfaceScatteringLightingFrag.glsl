#version 450
#pragma shader_stage(fragment)

precision highp float;

#include "SubSurfaceScatteringCommon"

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2D textures[];

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;

layout(push_constant) uniform PushConstants
{
    LightPushConstantData data;
}
pushConstants;

float sampleAOBlurred(vec2 uv)
{
    int kernelSize = 2;
    vec2 texelSize = vec2(1.0 / textureSize(textures[AVD_SSS_RENDER_MODE_SCENE_AO], 0));
    float ao       = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            ao += texture(textures[AVD_SSS_RENDER_MODE_SCENE_AO], uv + offset).r;
        }
    }
    ao /= float((2 * kernelSize + 1) * (2 * kernelSize + 1));
    return ao;
} 

void main()
{
    outDiffuse  = vec4(0.0, 0.0, 0.0, 1.0);
    outSpecular = vec4(0.0, 0.0, 0.0, 1.0);

    vec4 albedo = texture(textures[AVD_SSS_RENDER_MODE_SCENE_ALBEDO], inUV);

    if (albedo.a == 1) {
        outDiffuse = vec4(pushConstants.data.lightColor.rgb, 1.0);
    } else {
        vec3 viewPosition = texture(textures[AVD_SSS_RENDER_MODE_SCENE_POSITION], inUV).xyz;
        vec3 normal       = normalize(texture(textures[AVD_SSS_RENDER_MODE_SCENE_NORMAL], inUV).xyz * 2.0 - 1.0);
        vec3 viewDir      = normalize(-viewPosition);
        vec4 trm          = texture(textures[AVD_SSS_RENDER_MODE_SCENE_THICKNESS_ROUGHNESS_METALLIC], inUV);
        float sceneAo     = sampleAOBlurred(inUV);

        float roughness = trm.g + pushConstants.data.materialRoughness;
        float metallic  = trm.b + pushConstants.data.materialMetallic;
        vec3 F0         = vec3(0.04);
        F0              = mix(F0, albedo.rgb, metallic);

        float distortion       = pushConstants.data.translucencyDistortion;
        float ambientDiffusion = pushConstants.data.translucencyAmbientDiffusion;
        float thickness        = 1.0 - trm.r;
        float diffusionScale   = pushConstants.data.translucencyScale;
        float diffusionPower   = pushConstants.data.translucencyPower;

        vec3 diffuseLo  = vec3(0.0);
        vec3 specularLo = vec3(0.0);

        // // Standard PBR implementation from learnopengl.com
        for (int i = 0; i < 6; ++i) {
            vec3 lightPos    = pushConstants.data.lights[i].xyz;
            vec3 lightDirUn  = lightPos - viewPosition;
            vec3 lightDir    = normalize(lightDirUn);
            vec3 halfViewDir = normalize(lightDir + viewDir);

            float lightDist   = length(lightDirUn);
            float attenuation = 1.0 / (lightDist * lightDist);

            vec3 viewLightDir       = lightDir + normal * distortion;
            float FdotL             = pow(max(dot(viewDir, -viewLightDir), 0.0), diffusionPower) * diffusionScale;
            float NdotL             = max(dot(normal, lightDir), 0.0);
            float diffusionRadiance = attenuation * (FdotL + ambientDiffusion * sceneAo + NdotL) * thickness;

            // The Cook-Torrance BRDF
            float ndf    = distributionGGX(normal, halfViewDir, roughness);
            float g      = geometrySmith(normal, viewDir, lightDir, roughness);
            vec3 fresnel = fresnelSchlick(max(dot(halfViewDir, viewDir), 0.0), F0);

            vec3 numerator    = ndf * g * fresnel;
            float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 1e-4;
            vec3 specular     = numerator / denominator;

            // NOTE: I am skipping the frenel component here, even though it breaks
            // the law of conservation of energy but this to me looks visually more
            // interesting.
            vec3 kS = fresnel * 0.0;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;

            diffuseLo += kD * albedo.rgb / PI * (diffusionRadiance * pushConstants.data.lightColor.rgb);
            specularLo += specular * (diffusionRadiance * pushConstants.data.lightColor.rgb);
        }

        vec3 ambient = vec3(0.05) * albedo.rgb;
        diffuseLo += ambient * sceneAo;

        outDiffuse  = vec4(diffuseLo, 1.0);
        outSpecular = vec4(specularLo, 1.0);
    }
}
