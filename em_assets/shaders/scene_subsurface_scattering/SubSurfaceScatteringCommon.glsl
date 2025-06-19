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

struct ModelVertex {
    float16_t vx, vy, vz;
    uint16_t tp; // packed tangent: 8-8 octahedral
    uint np;     // packed normal: 10-10-10-2 vector + bitangent sign
    float16_t tu, tv;
};


#define PI 3.14159265

float linearizeDepth(float depth)
{
    float zNear = 0.1;   // Near plane distance
    float zFar  = 100.0; // Far plane distance
    return zNear * zFar / (zFar + depth * (zNear - zFar));
}


mat4 removeScaleFromMat4(mat4 m)
{
    mat4 result = m;
    result[0]   = vec4(normalize(m[0].xyz), 0.0);
    result[1]   = vec4(normalize(m[1].xyz), 0.0);
    result[2]   = vec4(normalize(m[2].xyz), 0.0);
    // The translation component (m[3]) is already correct.
    return result;
}


mat4 perspectiveMatrix(float fov, float aspect, float near, float far)
{
    float tanHalfFov = tan(fov / 2.0);
    return mat4(
        1.0 / (aspect * tanHalfFov), 0.0, 0.0, 0.0,
        0.0, 1.0 / tanHalfFov, 0.0, 0.0,
        0.0, 0.0, -(far + near) / (far - near), -1.0,
        0.0, 0.0, -(2.0 * far * near) / (far - near), 0.0);
}

// Source: https://github.com/zeux/niagara/blob/master/src/shaders/math.h
vec3 decodeOct(vec2 e)
{
	// https://x.com/Stubbesaurus/status/937994790553227264
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	float t = max(-v.z, 0);
	v.xy += vec2(v.x >= 0 ? -t : t, v.y >= 0 ? -t : t);
	return normalize(v);
}


void unpackTBN(uint np, uint tp, out vec3 normal, out vec4 tangent)
{
	normal = ((ivec3(np) >> ivec3(0, 10, 20)) & ivec3(1023)) / 511.0 - 1.0;
	tangent.xyz = decodeOct(((ivec2(tp) >> ivec2(0, 8)) & ivec2(255)) / 127.0 - 1.0);
	tangent.w = (np & (1 << 30)) != 0 ? -1.0 : 1.0;
}


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
