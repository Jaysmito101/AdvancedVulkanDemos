#version 450
#pragma shader_stage(fragment)

precision highp float;

#include "SubSurfaceScatteringCommon"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inPosition;
layout(location = 3) flat in int inRenderingLight;
// Extremely inefficient thing to do but well, this is just a demo.
layout(location = 4) in vec3 inLightAPos;
layout(location = 5) in vec3 inLightBPos;


layout(push_constant) uniform PushConstants
{
    UberPushConstantData data;
}
pushConstants;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outThicknessRoughnessMetallic;
layout(location = 3) out vec4 outViewPosition;


layout(set = 1, binding = 1) uniform sampler2D textures[];


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
    vec4 albedo = vec4(0.0);
    vec3 normal = vec3(0.0);
    vec4 trm    = vec4(0.0);


    vec2 uv = vec2(inUV.x, 1.0 - inUV.y);
    if (inRenderingLight == 1) {
        albedo = vec4(lightAColor, 1.0);
    } else if (inRenderingLight == 2) {
        albedo = vec4(lightBColor, 1.0);
    } else {
        albedo = vec4(1.0, 1.0, 1.0, 0.0);
        float thickness = texture(textures[pushConstants.data.thicknessMapIndex], uv).r;
        trm = vec4(thickness, 0.2, 0.6, 0.0);
        normal = normalize(inNormal);
        if (pushConstants.data.hasPBRTextures == 1) {
            albedo = texture(textures[pushConstants.data.albedoTextureIndex], uv);
            albedo.a = 0.0;
            vec3 normalTexture = texture(textures[pushConstants.data.normalTextureIndex], uv).rgb;
            vec3 normal = normalize(normalTexture * 2.0 - 1.0);
            mat3 TBN = calculateTBN();
            normal = normalize(TBN * normal);
        }
    }

    outAlbedo = albedo;
    outNormal = vec4(normal * 0.5 + 0.5, 1.0);
    outThicknessRoughnessMetallic = trm;
    outViewPosition = vec4(inPosition.xyz, 1.0);
}
