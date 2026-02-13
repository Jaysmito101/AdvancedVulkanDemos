#ifndef AVD_MATH_BASE_H
#define AVD_MATH_BASE_H

#include "core/avd_core.h"

// Define common constants
#define AVD_PI 3.14159265358979323846f
#define AVD_E  2.71828182845904523536f

// Define common macros for mathematical operations
#define avdDeg2Rad(deg)           ((deg) * (AVD_PI / 180.0f))
#define avdRad2Deg(rad)           ((rad) * (180.0f / AVD_PI))
#define avdClamp(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
#define avdMax(a, b)              ((a) > (b) ? (a) : (b))
#define avdMin(a, b)              ((a) < (b) ? (a) : (b))
#define avdAbs(value)             (fabsf(value))
#define avdSqrt(value)            sqrtf(value)
#define avdSin(value)             sinf(value)
#define avdCos(value)             cosf(value)
#define avdTan(value)             tanf(value)
#define avdLerp(a, b, t)          ((a) + ((b) - (a)) * (t))
#define avdIsFEqual(a, b)         (fabsf((a) - (b)) < 1e-6f && !isnan(a) && !isnan(b) && !isinf(a) && !isinf(b) && fabsf(a) < 1e6f)

#endif // AVD_MATH_BASE_H