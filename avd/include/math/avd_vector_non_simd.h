#ifndef AVD_VECTOR_NON_SIMD_H
#define AVD_VECTOR_NON_SIMD_H

#include "math/avd_math_base.h"


typedef union {
    AVD_Float v[2];
    struct {
        AVD_Float x;
        AVD_Float y;
    };
} AVD_Vector2;

typedef union {
    AVD_Float v[3];
    struct {
        AVD_Float x;
        AVD_Float y;
        AVD_Float z;
    };
    struct {
        AVD_Float r;
        AVD_Float g;
        AVD_Float b;
    };
} AVD_Vector3;

typedef union {
    AVD_Float v[4];
    struct {
        AVD_Float x;
        AVD_Float y;
        AVD_Float z;
        AVD_Float w;
    };
    struct {
        AVD_Float r;
        AVD_Float g;
        AVD_Float b;
        AVD_Float a;
    };
} AVD_Vector4;




#define avdVec2(vx, vy) ((AVD_Vector2){ .x = (vx), .y = (vy) })
#define avdVec2Zero() avdVec2(0.0f, 0.0f)
#define avdVec2One() avdVec2(1.0f, 1.0f)
#define avdVec2Add(a, b) avdVec2( (a).x + (b).x, (a).y + (b).y )
#define avdVec2Subtract(a, b) avdVec2( (a).x - (b).x, (a).y - (b).y )
#define avdVec2Scale(v, scalar) avdVec2( (v).x * (scalar), (v).y * (scalar) )
#define avdVec2LengthSq(v) ((v).x * (v).x + (v).y * (v).y)
#define avdVec2Length(v) (avdSqrt(avdVec2LengthSq(v)))
#define avdVec2Dot(v1, v2) ((v1).x * (v2).x + (v1).y * (v2).y)
#define avdVec2Cross(v1, v2) ((v1).x * (v2).y - (v1).y * (v2).x)
#define avdVec2Lerp(a, b, t) avdVec2( \
    avdLerp((a).x, (b).x, (t)), \
    avdLerp((a).y, (b).y, (t)) \
)
#define avdVec2FromPolar(angle, length) avdVec2( length * avdCos(angle), length * avdSin(angle) )
#define avdVec2Log(v) AVD_LOG("Vec2[x: %.2f, y: %.2f]", (v).x, (v).y)

#define avdVec3(vx, vy, vz) ((AVD_Vector3){ .x = (vx), .y = (vy), .z = (vz) })
#define avdVec3Zero() avdVec3(0.0f, 0.0f, 0.0f)
#define avdVec3One() avdVec3(1.0f, 1.0f, 1.0f)
#define avdVec3Add(a, b) avdVec3( (a).x + (b).x, (a).y + (b).y, (a).z + (b).z)
#define avdVec3Subtract(a, b) avdVec3( (a).x - (b).x, (a).y - (b).y, (a).z - (b).z )
#define avdVec3Scale(v, scalar) avdVec3( (v).x * (scalar), (v).y * (scalar), (v).z * (scalar) )
#define avdVec3LengthSq(v) ((v).x * (v).x + (v).y * (v).y + (v).z * (v).z)
#define avdVec3InvLength(v) (avdSqrt(avdVec3LengthSq(v)) > 0.0f ? 1.0f / avdSqrt(avdVec3LengthSq(v)) : 0.0f)
#define avdVec3Length(v) (avdSqrt(avdVec3LengthSq(v)))
#define avdVec3Dot(v1, v2) ((v1).x * (v2).x + (v1).y * (v2).y + (v1).z * (v2).z)
#define avdVec3Cross(v1, v2) avdVec3( \
    (v1).y * (v2).z - (v1).z * (v2).y, \
    (v1).z * (v2).x - (v1).x * (v2).z, \
    (v1).x * (v2).y - (v1).y * (v2).x \
)
#define avdVec3Lerp(a, b, t) avdVec3( \
    avdLerp((a).x, (b).x, (t)), \
    avdLerp((a).y, (b).y, (t)), \
    avdLerp((a).z, (b).z, (t)) \
)
#define avdVec3FromPolar(azimuth, elevation, length) avdVec3( \
    length * avdCos(elevation) * avdCos(azimuth), \
    length * avdCos(elevation) * avdSin(azimuth), \
    length * avdSin(elevation) \
)
#define avdVec3FromSpherical(azimuth, elevation, radius) avdVec3( \
    radius * avdCos(elevation) * avdCos(azimuth), \
    radius * avdCos(elevation) * avdSin(azimuth), \
    radius * avdSin(elevation) \
)
#define avdVec3Log(v) AVD_LOG("Vec3[x: %.2f, y: %.2f, z: %.2f]", (v).x, (v).y, (v).z)


#define avdVec4(vx, vy, vz, vw) ((AVD_Vector4){ .x = (vx), .y = (vy), .z = (vz), .w = (vw) })
#define avdVec4Zero() avdVec4(0.0f, 0.0f, 0.0f, 0.0f)
#define avdVec4One() avdVec4(1.0f, 1.0f, 1.0f, 1.0f)
#define avdVec4FromVec3(v, w) avdVec4((v).x, (v).y, (v).z, (w))
#define avdVec4Add(a, b) avdVec4( (a).x + (b).x, (a).y + (b).y, (a).z + (b).z, (a).w + (b).w )
#define avdVec4Subtract(a, b) avdVec4( (a).x - (b).x, (a).y - (b).y, (a).z - (b).z, (a).w - (b).w )
#define avdVec4Scale(v, scalar) avdVec4( (v).x * (scalar), (v).y * (scalar), (v).z * (scalar), (v).w * (scalar) )
#define avdVec4LengthSq(v) ((v).x * (v).x + (v).y * (v).y + (v).z * (v).z + (v).w * (v).w)
#define avdVec4Length(v) (avdSqrt(avdVec4LengthSq(v)))
#define avdVec4InvLength(v) (avdSqrt(avdVec4LengthSq(v)) > 0.0f ? 1.0f / avdSqrt(avdVec4LengthSq(v)) : 0.0f)
// NOTE: Only calculates the 3D part of the vector
#define avdVec4Cross(v1, v2) avdVec4( \
    (v1).y * (v2).z - (v1).z * (v2).y, \
    (v1).z * (v2).x - (v1).x * (v2).z, \
    (v1).x * (v2).y - (v1).y * (v2).x, \
    1.0f \
)
#define avdVec4Lerp(a, b, t) avdVec4( \
    avdLerp((a).x, (b).x, (t)), \
    avdLerp((a).y, (b).y, (t)), \
    avdLerp((a).z, (b).z, (t)), \
    avdLerp((a).w, (b).w, (t)) \
)
#define avdVec4Log(v) AVD_LOG("Vec4[x: %.2f, y: %.2f, z: %.2f, w: %.2f]", (v).x, (v).y, (v).z, (v).w)

// Define the swizzle macros for vectors

#define avdVecSwizzle2(v, c1, c2) avdVec2((v). c1, (v). c2)
#define avdVecSwizzle3(v, c1, c2, c3) avdVec3((v). c1, (v). c2, (v). c3)
#define avdVecSwizzle4(v, c1, c2, c3, c4) avdVec4((v). c1, (v). c2, (v). c3, (v). c4)


// Create static functions for some operations that are more optimized this way
static inline AVD_Vector2 avdVec2Normalize(const AVD_Vector2 v) {
    AVD_Float length = avdVec2Length(v);
    return length > 0.0f ? avdVec2Scale(v, 1.0f / length) : avdVec2Zero();
}

static inline AVD_Vector3 avdVec3Normalize(const AVD_Vector3 v) {
    AVD_Float length = avdVec3Length(v);
    return length > 0.0f ? avdVec3Scale(v, 1.0f / length) : avdVec3Zero();
}

static inline AVD_Vector4 avdVec4Normalize(const AVD_Vector4 v) {
    AVD_Float length = avdVec4Length(v);
    return length > 0.0f ? avdVec4Scale(v, 1.0f / length) : avdVec4Zero();
}



#endif // AVD_VECTOR_NON_SIMD_H