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
#else
    #error "Shader language not defined. Please define either AVD_HLSL or AVD_GLSL"
#endif


#ifdef AVD_HLSL
    #define mix lerp
    #define mod fmod
    #define fract frac

    #define SAMPLE_TEXTURE(tex, uv) tex.Sample(tex##_sampler, uv)

#else 
    #define saturate(x) clamp(x, 0.0, 1.0)

    #define SAMPLE_TEXTURE(tex, uv) texture(tex, uv)
#endif


#endif 