
#version 450

#pragma shader_stage(vertex)

#include "SubSurfaceScatteringCommon"

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outPosition;
layout(location = 3) flat out int outRenderingLight;
// Extremely inefficient thing to do but well, this is just a demo.
layout(location = 4) out vec3 outLightAPos;
layout(location = 5) out vec3 outLightBPos;

layout(set = 0, binding = 0, std430) readonly buffer VertexBuffer
{
    ModelVertex vertices[];
};

layout(push_constant) uniform PushConstants
{
    UberPushConstantData data;
}
pushConstants;

vec4 samplePosition(uint index)
{
    return vec4(vertices[index].vx, vertices[index].vy, vertices[index].vz, 1.0);
} 

vec2 sampleTextureCoords(uint index)
{
    return vec2(vertices[index].tu, vertices[index].tv);
}

void main()
{ 
    outUV = vec2(0.0, 0.0); // Initialize outUV to avoid warnings

    int vertexIndex = gl_VertexIndex + pushConstants.data.vertexOffset;

    mat4 viewModelMatrix = pushConstants.data.viewModelMatrix;
    mat4 projectionMatrix = pushConstants.data.projectionMatrix;

    mat3 normalMatrix     = transpose(inverse(mat3(pushConstants.data.viewModelMatrix)));

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
        // modelMatrix = unscaledModel;
    } else {
        outRenderingLight = 0;
        vertexPosition    = samplePosition(vertexIndex);
    }

    vec4 viewPosition = viewModelMatrix * vec4(vertexPosition.xyz, 1.0);
    vec4 position      = projectionMatrix * viewPosition;

    vec3 normal;
    vec4 tangent;
    unpackTBN(vertices[vertexIndex].np, uint(vertices[vertexIndex].tp), normal, tangent);

    // Set the output variables
    outUV        = sampleTextureCoords(vertexIndex);
    outPosition  = viewPosition;
    outNormal    = normalMatrix * normal;
    // outLightAPos = (unscaledModel * pushConstants.data.lightA).xyz;
    // outLightBPos = (unscaledModel * pushConstants.data.lightB).xyz;

    // Set the gl_Position for the vertex shader
    gl_Position = position;
}