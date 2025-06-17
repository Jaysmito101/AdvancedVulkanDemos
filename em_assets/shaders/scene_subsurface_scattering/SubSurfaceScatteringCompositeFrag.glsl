#version 450
#pragma shader_stage(fragment)

precision highp float;

#define AVD_SSS_RENDER_MODE_RESULT                    0
#define AVD_SSS_RENDER_MODE_SCENE_DEPTH               1
#define AVD_SSS_RENDER_MODE_SCENE_AO                  2
#define AVD_SSS_RENDER_MODE_SCENE_DIFFUSE             3
#define AVD_SSS_RENDER_MODE_SCENE_SPECULAR            4
#define AVD_SSS_RENDER_MODE_SCENE_EMISSIVE            5
#define AVD_SSS_RENDER_MODE_SCENE_DIFFUSED_IRRADIANCE 6
#define AVD_SSS_ALIEN_THICKNESS_MAP                   7
#define AVD_SSS_BUDDHA_THICKNESS_MAP                  8
#define AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP         8
#define AVD_SSS_BUDDHA_ORM_MAP                        10
#define AVD_SSS_BUDDHA_ALBEDO_MAP                     11
#define AVD_SSS_BUDDHA_NORMAL_MAP                     12
#define AVD_SSS_NOISE_TEXTURE                         13
#define AVD_SSS_RENDER_MODE_COUNT                     14

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D textures[];

struct PushConstantData {
    int renderMode;
};

layout(push_constant) uniform PushConstants
{
    PushConstantData data;
}
pushConstants;

float linearizeDepth(float depth)
{
    float zNear = 0.1;   // Near plane distance
    float zFar  = 100.0; // Far plane distance
    return zNear * zFar / (zFar - depth * (zFar - zNear));
}

void main()
{
    vec2 uv = vec2(inUV.x, 1.0 - inUV.y); 

    int renderMode = pushConstants.data.renderMode;

    if (renderMode == AVD_SSS_RENDER_MODE_RESULT) {
        vec4 diffuse = texture(textures[3], uv);
        vec4 specular = texture(textures[4], uv);
        vec4 emissive = texture(textures[5], uv);
        outColor = vec4(diffuse.rgb + specular.rgb + emissive.rgb, 1.0);
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_DEPTH) {
        float depth = texture(textures[1], uv).r;
        outColor    = vec4(vec3(linearizeDepth(depth)), 1.0);
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_AO) {
        outColor = texture(textures[2], uv);
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_DIFFUSE) {
        outColor = texture(textures[3], uv);
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_SPECULAR) {
        outColor = texture(textures[4], uv);
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_EMISSIVE) {
        outColor = texture(textures[5], uv);
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_DIFFUSED_IRRADIANCE) {
        outColor = texture(textures[6], uv);
    } else if (renderMode == AVD_SSS_ALIEN_THICKNESS_MAP) {
        outColor = texture(textures[8], uv);
    } else if (renderMode == AVD_SSS_BUDDHA_THICKNESS_MAP) {
        outColor = texture(textures[9], uv);
    } else if (renderMode == AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP) {
        outColor = texture(textures[10], uv);
    } else if (renderMode == AVD_SSS_BUDDHA_ORM_MAP) {
        outColor = texture(textures[11], uv);
    } else if (renderMode == AVD_SSS_BUDDHA_ALBEDO_MAP) {
        outColor = texture(textures[12], uv);
    } else if (renderMode == AVD_SSS_BUDDHA_NORMAL_MAP) {
        outColor = texture(textures[13], uv);
    } else if (renderMode == AVD_SSS_NOISE_TEXTURE) {
        outColor = vec4(texture(textures[14], uv).rgb, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0); // Fallback color
    }
}
