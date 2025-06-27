#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require

#define AVD_SSS_RENDER_MODE_RESULT                             0
#define AVD_SSS_RENDER_MODE_SCENE_ALBEDO                       1
#define AVD_SSS_RENDER_MODE_SCENE_NORMAL                       2
#define AVD_SSS_RENDER_MODE_SCENE_THICKNESS_ROUGHNESS_METALLIC 3
#define AVD_SSS_RENDER_MODE_SCENE_POSITION                     4
#define AVD_SSS_RENDER_MODE_SCENE_DEPTH                        5
#define AVD_SSS_RENDER_MODE_SCENE_AO                           6
#define AVD_SSS_RENDER_MODE_SCENE_DIFFUSE                      7
#define AVD_SSS_RENDER_MODE_SCENE_SPECULAR                     8
#define AVD_SSS_RENDER_MODE_SCENE_DIFFUSED_IRRADIANCE          9
#define AVD_SSS_ALIEN_THICKNESS_MAP                            10
#define AVD_SSS_BUDDHA_THICKNESS_MAP                           11
#define AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP                  12
#define AVD_SSS_BUDDHA_ORM_MAP                                 13
#define AVD_SSS_BUDDHA_ALBEDO_MAP                              14
#define AVD_SSS_BUDDHA_NORMAL_MAP                              15
#define AVD_SSS_NOISE_TEXTURE                                  16
#define AVD_SSS_RENDER_MODE_COUNT                              17

#include "MeshUtils"
#include "MathUtils"
#include "PBRUtils"

struct UberPushConstantData {
    mat4 viewModelMatrix;
    mat4 projectionMatrix;

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

struct LightPushConstantData {
    vec4 lights[6];
    vec4 cameraPosition;
    vec4 screenSizes;
    vec4 lightColor;

    float materialRoughness;
    float materialMetallic;
    float translucencyScale;
    
    float translucencyDistortion;
    float translucencyPower;
    float translucencyAmbientDiffusion;
    float screenSpaceIrradianceScale;    
};


struct CompositePushConstantData {
    int renderMode;
    int useScreenSpaceIrradiance;
};



