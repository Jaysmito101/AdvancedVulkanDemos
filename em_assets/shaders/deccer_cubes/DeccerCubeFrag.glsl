#version 450
#pragma shader_stage(fragment)

precision highp float;

#include "DeccerCubeCommon"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inPosition;

layout(push_constant) uniform PushConstants
{
    UberPushConstantData data;
}
pushConstants;

layout(location = 0) out vec4 outFragColor;

layout(set = 1, binding = 1) uniform sampler2D textures[];

void main()
{
    const vec3 lightPosition = vec3(5.0, 5.0, 5.0);

    vec3 N = normalize(inNormal);
    vec3 L = normalize(lightPosition - inPosition.xyz);
    
    vec3 albedo = vec3(0.8, 0.3, 0.2);
    if (pushConstants.data.textureIndex >= 0) {
        albedo = texture(textures[pushConstants.data.textureIndex], inUV).rgb;
    }

    vec3 ambient = 0.05 * albedo;
    vec3 diffuse = max(dot(N, L), 0.0) * albedo;

    vec3 color = ambient + diffuse;
    outFragColor = vec4(color, 1.0);
}
