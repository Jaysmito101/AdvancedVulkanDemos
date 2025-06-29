#ifndef MATH_UTILS
#define MATH_UTILS

#include "ShaderAdapter"

#define PI 3.14159265

float linearizeDepth(float depth)
{
    float zNear = 0.1;   // Near plane distance
    float zFar  = 100.0; // Far plane distance
    return zNear * zFar / (zFar + depth * (zNear - zFar));
}


float4x4 removeScaleFromMat4(float4x4 m)
{
    float4x4 result = m;
    result[0]   = float4(normalize(m[0].xyz), 0.0);
    result[1]   = float4(normalize(m[1].xyz), 0.0);
    result[2]   = float4(normalize(m[2].xyz), 0.0);
    // The translation component (m[3]) is already correct.
    return result;
}


float4x4 perspectiveMatrix(float fov, float aspect, float near, float far)
{
    float tanHalfFov = tan(fov / 2.0);
    return float4x4(
        1.0 / (aspect * tanHalfFov), 0.0, 0.0, 0.0,
        0.0, 1.0 / tanHalfFov, 0.0, 0.0,
        0.0, 0.0, -(far + near) / (far - near), -1.0,
        0.0, 0.0, -(2.0 * far * near) / (far - near), 0.0);
}

float3 hashToColor(uint seed) {
    float3 color = float3(0.5 + 0.5 * sin(float3(seed, seed * 1.6180339887, seed * 2.7182818284) + float3(0, 2, 4)));
    return color; 
}


#endif 