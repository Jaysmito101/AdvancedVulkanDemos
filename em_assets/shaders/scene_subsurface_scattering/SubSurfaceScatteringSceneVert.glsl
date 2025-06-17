
#version 450

#pragma shader_stage(vertex)

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBitangent;
layout(location = 4) out vec4 outPosition;
layout(location = 5) flat out int outRenderingLight;
// Extremely inefficient thing to do but well, this is just a demo.
layout(location = 6) out vec3 outLightAPos;
layout(location = 7) out vec3 outLightBPos;

struct ModelVertex {
    vec4 position;
    vec4 normal;
    vec4 texCoord;
    vec4 tangent;
    vec4 bitangent;
};

layout(set = 0, binding = 0, std430) readonly buffer VertexBuffer
{
    ModelVertex vertices[];
};

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

mat4 removeScaleFromMat4(mat4 m)
{
    mat4 result = m;
    result[0]   = vec4(normalize(m[0].xyz), 0.0);
    result[1]   = vec4(normalize(m[1].xyz), 0.0);
    result[2]   = vec4(normalize(m[2].xyz), 0.0);
    // The translation component (m[3]) is already correct.
    return result;
}

void main()
{
    outUV = vec2(0.0, 0.0); // Initialize outUV to avoid warnings

    int vertexIndex = gl_VertexIndex + pushConstants.data.vertexOffset;

    mat4 unscaledModel = removeScaleFromMat4(pushConstants.data.modelMatrix);
    mat4 viewModel     = pushConstants.data.viewMatrix * pushConstants.data.modelMatrix;
    mat4 projection    = pushConstants.data.projectionMatrix;
    mat3 normalMatrix  = transpose(inverse(mat3(pushConstants.data.modelMatrix)));

    vec4 vertexPosition = vec4(0.0);
    if (pushConstants.data.renderingLight == 1) {
        bool isLightA            = gl_VertexIndex / pushConstants.data.vertexCount == 0;
        vertexIndex              = gl_VertexIndex % pushConstants.data.vertexCount + pushConstants.data.vertexOffset;
        vec4 lightVertexPosition = vertices[vertexIndex].position * 0.1f;
        if (isLightA) {
            outRenderingLight = 1;
            vertexPosition    = pushConstants.data.lightA + lightVertexPosition;
        } else {
            outRenderingLight = 2;
            vertexPosition    = pushConstants.data.lightB + lightVertexPosition;
        }
        viewModel = pushConstants.data.viewMatrix * unscaledModel;
    } else {
        outRenderingLight = 0;
        vertexPosition    = vertices[vertexIndex].position;
    }

    vec4 worldPosition = pushConstants.data.modelMatrix * vertexPosition;
    vec4 position      = projection * viewModel * vec4(vertexPosition.xyz, 1.0);

    // Set the output variables
    outNormal    = normalMatrix * vertices[vertexIndex].normal.xyz;
    outTangent   = normalMatrix * vertices[vertexIndex].tangent.xyz;
    outBitangent = normalMatrix * vertices[vertexIndex].bitangent.xyz;
    outPosition  = worldPosition;
    outLightAPos = (unscaledModel * pushConstants.data.lightA).xyz;
    outLightBPos = (unscaledModel * pushConstants.data.lightB).xyz;

    // Set the gl_Position for the vertex shader
    gl_Position = position;
}