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

struct ModelVertex {
    float16_t vx, vy, vz;
    uint16_t tp; // packed tangent: 8-8 octahedral
    uint np;     // packed normal: 10-10-10-2 vector + bitangent sign
    float16_t tu, tv;
};

struct Light {
    vec3 position;
    vec3 color;
};


// light a slight yellow sun color
// light b sky color
const vec3 lightAColor = vec3(1.0, 1.0, 0.4) * 12.0;
const vec3 lightBColor = vec3(0.4, 0.6, 0.8);
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

