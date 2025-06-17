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
#define AVD_SSS_ALIEN_THICKNESS_MAP                   8
#define AVD_SSS_BUDDHA_THICKNESS_MAP                  9
#define AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP         10
#define AVD_SSS_RENDER_MODE_COUNT                     11

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
    vec2 uv = (inUV * 2.0 - 1.0);
    int renderMode = pushConstants.data.renderMode;
        
    if (renderMode == AVD_SSS_RENDER_MODE_RESULT) {
        // TEST 
        outColor = texture(textures[3], vec2(inUV.x, 1.0 - inUV.y));
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_DEPTH) {
        float depth = texture(textures[1], vec2(inUV.x, 1.0 - inUV.y)).r;
        outColor    = vec4(vec3(linearizeDepth(depth)), 1.0);
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_AO) {
        outColor = texture(textures[2], vec2(inUV.x, 1.0 - inUV.y));
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_DIFFUSE) {
        outColor = texture(textures[3], vec2(inUV.x, 1.0 - inUV.y));
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_SPECULAR) {
        outColor = texture(textures[4], vec2(inUV.x, 1.0 - inUV.y));
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_EMISSIVE) {
        outColor = texture(textures[5], vec2(inUV.x, 1.0 - inUV.y));
    } else if (renderMode == AVD_SSS_RENDER_MODE_SCENE_DIFFUSED_IRRADIANCE) {
        outColor = texture(textures[6], vec2(inUV.x, 1.0 - inUV.y));
    } else if (renderMode == AVD_SSS_ALIEN_THICKNESS_MAP) {
        outColor = texture(textures[8], vec2(inUV.x, 1.0 - inUV.y));
    } else if (renderMode == AVD_SSS_BUDDHA_THICKNESS_MAP) {
        outColor = texture(textures[9], vec2(inUV.x, 1.0 - inUV.y));
    } else if (renderMode == AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP) {
        outColor = texture(textures[10], vec2(inUV.x, 1.0 - inUV.y));
    }
}
