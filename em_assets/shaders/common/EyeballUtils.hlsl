#ifndef EYEBALL_UTILS_GLSL
#define EYEBALL_UTILS_GLSL

#ifdef AVD_GLSL
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#endif

#include "ShaderAdapter"
#include "MeshUtils"

struct EyeballUniforms {
    float4 eyePosition;
    float4 eyeRotation;
    float4x4 cameraMatrix;
    float radius;
};


#ifndef EYEBALL_SET
#define EYEBALL_SET 0
#endif

#ifdef AVD_GLSL
layout(set = EYEBALL_SET, binding = 0) uniform EyeballUniformBlock { 
    EyeballUniforms eyeballUniforms; 
}; 
    
layout(set = EYEBALL_SET, binding = 1) uniform sampler2D veinsTexture; 
    
layout(set = EYEBALL_SET, binding = 2, std430) readonly buffer EyeballVertexBuffer 
{ 
    ModelVertex eyeballVertices[]; 
}; 
    
layout(set = EYEBALL_SET, binding = 3) readonly buffer EyeballIndexBuffer
{ 
    uint eyeballIndices[];
}

#elif defined(AVD_HLSL)

[[vk::binding(0, EYEBALL_SET)]]
cbuffer EyeballUniformBlock {
    EyeballUniforms eyeballUniforms;
};

[[vk::binding(1, EYEBALL_SET)]]
Texture2D veinsTexture;

[[vk::binding(2, EYEBALL_SET)]]
StructuredBuffer<ModelVertex> eyeballVertices;

[[vk::binding(3, EYEBALL_SET)]]
StructuredBuffer<uint> eyeballIndices;


#endif

#endif