#version 450
#pragma shader_stage(fragment)

precision highp float;

#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inPosition;
layout(location = 3) flat in int inRenderingLight;
// Extremely inefficient thing to do but well, this is just a demo.
layout(location = 4) in vec3 inLightAPos;
layout(location = 5) in vec3 inLightBPos;

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;
layout(location = 2) out vec4 outEmissive;

layout(set = 1, binding = 1) uniform sampler2D textures[];

#define PI 3.14159265359

struct PushConstantData {
    mat4 modelMatrix;
    mat4 viewProjectionMatrix;

    vec4 lightA;
    vec4 lightB;
    vec4 cameraPosition;
    vec4 screenSizes;

    int vertexOffset;
    int vertexCount;
    int renderingLight;
    int hasPBRTextures;

    uint albedoTextureIndex;
    uint normalTextureIndex;
    uint ormTextureIndex;
    uint thicknessMapIndex;
};

layout(push_constant) uniform PushConstants
{
    PushConstantData data;
}
pushConstants;


float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Soruce: https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
mat3 calculateTBN()
{
    vec3 Q1  = dFdx(inPosition.xyz);
    vec3 Q2  = dFdy(inPosition.xyz);
    vec2 st1 = dFdx(inUV);
    vec2 st2 = dFdy(inUV);

    vec3 N   = normalize(inNormal);
    vec3 T   = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return TBN;
}

void main()
{
    outDiffuse  = vec4(0.0, 0.0, 0.0, 1.0);
    outSpecular = vec4(0.0, 0.0, 0.0, 1.0);
    outEmissive = vec4(0.0, 0.0, 0.0, 1.0);

    if (inRenderingLight == 1) {
        outEmissive = vec4(lightAColor, 1.0);
    } else if (inRenderingLight == 2) {
        outEmissive = vec4(lightBColor, 1.0);
    } else {
        Light lights[2];
        lights[0].position = inLightAPos;
        lights[0].color    = lightAColor;
        lights[1].position = inLightBPos;
        lights[1].color    = lightBColor;

        vec3 viewDir = normalize(pushConstants.data.cameraPosition.xyz - inPosition.xyz);
        vec3 normal  = normalize(inNormal);

        vec3 albedo     = vec3(0.5, 0.6, 0.7);
        float roughness = 0.2;
        float metallic  = 0.6;
        vec3 F0         = vec3(0.04);

        if (pushConstants.data.hasPBRTextures == 1) {
            vec2 uv  = vec2(inUV.x, 1.0 - inUV.y);
            mat3 TBN = calculateTBN();

            albedo    = texture(textures[pushConstants.data.albedoTextureIndex], uv).rgb;
            normal    = normalize(TBN * texture(textures[pushConstants.data.normalTextureIndex], uv).rgb);
            // roughness = texture(textures[pushConstants.data.ormTextureIndex], uv).g;
            // metallic  = texture(textures[pushConstants.data.ormTextureIndex], uv).b;
            // F0        = mix(F0, albedo, metallic);
        }

        vec3 diffuseLo  = vec3(0.0);
        vec3 specularLo = vec3(0.0);

        // Standard PBR implementation from learnopengl.com
        for (int i = 0; i < 2; ++i) {
            vec3 lightDir    = normalize(lights[i].position - inPosition.xyz);
            vec3 halfViewDir = normalize(lightDir + viewDir);

            float lightDist   = length(lights[i].position - inPosition.xyz);
            float attenuation = 1.0 / (lightDist * lightDist);
            vec3 radiance     = lights[i].color * attenuation;

            // The Cook-Torrance BRDF
            float ndf    = distributionGGX(normal, halfViewDir, roughness);
            float g      = geometrySmith(normal, viewDir, lightDir, roughness);
            vec3 fresnel = fresnelSchlick(max(dot(halfViewDir, viewDir), 0.0), F0);

            vec3 numerator    = ndf * g * fresnel;
            float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 1e-4;
            vec3 specular     = numerator / denominator;

            vec3 kS = fresnel;

            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;

            float NdotL = max(dot(normal, lightDir), 0.0);

            diffuseLo += kD * albedo / PI * radiance * NdotL;
            specularLo += specular * radiance * NdotL;
        }

        vec3 ambient = vec3(0.03) * albedo;
        diffuseLo += ambient;

        outDiffuse  = vec4(diffuseLo, 1.0);
        outSpecular = vec4(specularLo, 1.0);
        outEmissive = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
