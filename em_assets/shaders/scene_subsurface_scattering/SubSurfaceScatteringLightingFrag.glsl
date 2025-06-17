#version 450
#pragma shader_stage(fragment)

precision highp float;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec4 inPosition;
layout(location = 5) flat in int inRenderingLight;
// Extremely inefficient thing to do but well, this is just a demo.
layout(location = 6) in vec3 inLightAPos;
layout(location = 7) in vec3 inLightBPos;

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;
layout(location = 2) out vec4 outEmissive;

#define PI 3.14159265359

struct PushConstantData {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;

    vec4 lightA;
    vec4 lightB;
    vec4 cameraPosition;

    int vertexOffset;
    int vertexCount;
    int renderingLight;
    int pad1;
};

layout(push_constant) uniform PushConstants
{
    PushConstantData data;
}
pushConstants;

// light a slight yellow sun color
// light b sky color
const vec3 lightAColor = vec3(1.0, 1.0, 0.8) * 25.0;
const vec3 lightBColor = vec3(0.8, 0.9, 1.0) * 20.0;

struct Light {
    vec3 position;
    vec3 color;
};

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    outDiffuse = vec4(0.0, 0.0, 0.0, 1.0);
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
        vec3 normal = normalize(inNormal);

        vec3 albedo = vec3(0.5, 0.6, 0.7);
        float roughness = 0.5; 
        float metallic = 0.0;
        vec3 F0 = vec3(0.04);

        vec3 diffuseLo = vec3(0.0);
        vec3 specularLo = vec3(0.0);
 
        // Standard PBR implementation from learnopengl.com
        for (int i = 0; i < 2; ++i) {
            vec3 lightDir = normalize(lights[i].position - inPosition.xyz);
            vec3 halfViewDir = normalize(lightDir + viewDir);

            float lightDist = length(lights[i].position - inPosition.xyz);
            float attenuation = 1.0 / (lightDist * lightDist);
            vec3 radiance = lights[i].color * attenuation;

            // The Cook-Torrance BRDF
            float ndf = distributionGGX(normal, halfViewDir, roughness);
            float g = geometrySmith(normal, viewDir, lightDir, roughness);
            vec3 fresnel = fresnelSchlick(max(dot(halfViewDir, viewDir), 0.0), F0);

            vec3 numerator = ndf * g * fresnel;
            float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 1e-4;
            vec3 specular = numerator / denominator;

            vec3 kS = fresnel;

            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;

            float NdotL = max(dot(normal, lightDir), 0.0);

            diffuseLo += kD * albedo / PI * radiance * NdotL;
            specularLo += specular * radiance * NdotL;
        }

        vec3 ambient = vec3(0.03) * albedo;
        diffuseLo += ambient;

        outDiffuse = vec4(diffuseLo, 1.0);
        outSpecular = vec4(specularLo, 1.0);
        outEmissive = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
