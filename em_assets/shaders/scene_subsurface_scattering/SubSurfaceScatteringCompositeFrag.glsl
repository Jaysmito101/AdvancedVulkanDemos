#version 450
#pragma shader_stage(fragment)

precision highp float;

#include "SubSurfaceScatteringCommon"

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D textures[];


layout(push_constant) uniform PushConstants
{
    CompositePushConstantData data;
}
pushConstants;

void main()
{
    vec2 uv = vec2(inUV.x, 1.0 - inUV.y);
 
    int renderMode = pushConstants.data.renderMode;

    if (renderMode == AVD_SSS_RENDER_MODE_RESULT) {
        vec4 diffuse = vec4(0.0);
        if (pushConstants.data.useScreenSpaceIrradiance == 1) {
            diffuse = texture(textures[AVD_SSS_RENDER_MODE_SCENE_DIFFUSED_IRRADIANCE], uv);
        } else {
            diffuse = texture(textures[AVD_SSS_RENDER_MODE_SCENE_DIFFUSE], uv);
        }
        vec4 specular = texture(textures[AVD_SSS_RENDER_MODE_SCENE_SPECULAR], uv);
        outColor      = vec4(diffuse.rgb + specular.rgb, 1.0);
    } else {
        vec3 color = texture(textures[renderMode], uv).rgb;

        if (renderMode == AVD_SSS_RENDER_MODE_SCENE_DEPTH) {
            color = vec3(linearizeDepth(color.r));
        } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_AO) {
            color = vec3(color.r);
        }

        outColor = vec4(color, 1.0);
    }
}
