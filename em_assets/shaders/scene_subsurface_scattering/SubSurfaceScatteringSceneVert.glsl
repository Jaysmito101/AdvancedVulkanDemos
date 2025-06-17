
#version 450

#pragma shader_stage(vertex)

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outPosition;
layout(location = 3) flat out int outRenderingLight;
// Extremely inefficient thing to do but well, this is just a demo.
layout(location = 4) out vec3 outLightAPos;
layout(location = 5) out vec3 outLightBPos;

struct ModelVertex {
    float16_t vx, vy, vz;
    uint16_t tp; // packed tangent: 8-8 octahedral
    uint np;     // packed normal: 10-10-10-2 vector + bitangent sign
    float16_t tu, tv;
};

layout(set = 0, binding = 0, std430) readonly buffer VertexBuffer
{
    ModelVertex vertices[];
};

struct PushConstantData {
    mat4 modelMatrix;
    mat4 viewProjectionMatrix;

    vec4 lightA;
    vec4 lightB;
    vec4 cameraPosition;

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

mat4 removeScaleFromMat4(mat4 m)
{
    mat4 result = m;
    result[0]   = vec4(normalize(m[0].xyz), 0.0);
    result[1]   = vec4(normalize(m[1].xyz), 0.0);
    result[2]   = vec4(normalize(m[2].xyz), 0.0);
    // The translation component (m[3]) is already correct.
    return result;
}

vec4 samplePosition(uint index)
{
    return vec4(vertices[index].vx, vertices[index].vy, vertices[index].vz, 1.0);
}

vec2 sampleTextureCoords(uint index)
{
    return vec2(vertices[index].tu, vertices[index].tv);
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

void main()
{
    outUV = vec2(0.0, 0.0); // Initialize outUV to avoid warnings

    int vertexIndex = gl_VertexIndex + pushConstants.data.vertexOffset;

    mat4 unscaledModel  = removeScaleFromMat4(pushConstants.data.modelMatrix);
    mat4 modelMatrix    = pushConstants.data.modelMatrix;
    mat4 viewProjection = pushConstants.data.viewProjectionMatrix;
    mat3 normalMatrix   = transpose(inverse(mat3(pushConstants.data.modelMatrix)));

    vec4 vertexPosition = vec4(0.0);
    if (pushConstants.data.renderingLight == 1) {
        bool isLightA            = gl_VertexIndex / pushConstants.data.vertexCount == 0;
        vertexIndex              = gl_VertexIndex % pushConstants.data.vertexCount + pushConstants.data.vertexOffset;
        vec4 lightVertexPosition = samplePosition(vertexIndex) * 0.1f;
        if (isLightA) {
            outRenderingLight = 1;
            vertexPosition    = pushConstants.data.lightA + lightVertexPosition;
        } else {
            outRenderingLight = 2;
            vertexPosition    = pushConstants.data.lightB + lightVertexPosition;
        }
        modelMatrix = unscaledModel;
    } else {
        outRenderingLight = 0;
        vertexPosition    = samplePosition(vertexIndex);
    }

    vec4 worldPosition = pushConstants.data.modelMatrix * vertexPosition;
    vec4 position      = viewProjection * modelMatrix * vec4(vertexPosition.xyz, 1.0);

    vec3 normal;
    vec4 tangent;
    unpackTBN(vertices[vertexIndex].np, uint(vertices[vertexIndex].tp), normal, tangent);

    // Set the output variables
    outUV        = sampleTextureCoords(vertexIndex);
    outPosition  = worldPosition;
    outNormal    = normalMatrix * normal;
    outLightAPos = (unscaledModel * pushConstants.data.lightA).xyz;
    outLightBPos = (unscaledModel * pushConstants.data.lightB).xyz;

    // Set the gl_Position for the vertex shader
    gl_Position = position;
}