#ifndef TONEMAP_UTILS_HLSL
#define TONEMAP_UTILS_HLSL

#include "ShaderAdapter"
#include "MathUtils"

float3 acesFilm(float3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}


#endif // TONEMAP_UTILS_HLSL