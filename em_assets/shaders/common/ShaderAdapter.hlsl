#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H


#ifdef AVD_HLSL
    #define vec2 float2
    #define vec3 float3
    #define vec4 float4
    #define ivec2 int2
    #define ivec3 int3
    #define ivec4 int4
    
    #define mat2 float2x2
    #define mat3 float3x3
    #define mat4 float4x4

    #define float16_t min16float
    #define int16_t min16int
    #define uint16_t min16uint


    #define SAMPLER2D(name, reg) Texture2D name : register(t##reg); SamplerState name##_sampler : register(s##reg)
    #define SAMPLER2D_TAB(name, reg) Texture2D name[] : register(t##reg); SamplerState name##_sampler[] : register(s##reg)

#elif defined(AVD_GLSL)
    #define float2 vec2
    #define float3 vec3
    #define float4 vec4
    #define int2 ivec2
    #define int3 ivec3
    #define int4 ivec4
    #define float2x2 mat2
    #define float3x3 mat3
    #define float4x4 mat4

    #define min16float float16_t
    #define min16int int16_t
    #define min16uint uint16_t
    
    #define SAMPLER2D(name, binding_loc) layout(binding = binding_loc) uniform sampler2D name
    #define SAMPLER2D_TAB(name, binding_loc) layout(binding = binding_loc) uniform sampler2D name[];
#else
    #error "Shader language not defined. Please define either AVD_HLSL or AVD_GLSL"
#endif


#ifdef AVD_HLSL
    #define mix lerp
    #define mod fmod
    #define fract frac

    #define SAMPLE_TEXTURE(tex, uv) tex.Sample(tex##_sampler, uv)
    #define SAMPLE_TEXTURE_TAB(tex, uv, index) tex[index].Sample(tex##_sampler[index], uv)
#else 
    #define saturate(x) clamp(x, 0.0, 1.0)

    #define SAMPLE_TEXTURE(tex, uv) texture(tex, uv)
    #define SAMPLE_TEXTURE_TAB(tex, uv, index) texture(tex[index], uv)
#endif


#ifdef AVD_HLSL

float3x3 inverse(float3x3 m) {
    float3x3 adj;
    adj[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    adj[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
    adj[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    adj[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    adj[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
    adj[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
    adj[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];
    adj[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];
    adj[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];

    float det = dot(m[0], float3(adj[0][0], adj[1][0], adj[2][0]));
    return adj / det;
}

#endif

#endif 