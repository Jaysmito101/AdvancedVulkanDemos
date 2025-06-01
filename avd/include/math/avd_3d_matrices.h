#ifndef AVD_3D_MATRICES_H
#define AVD_3D_MATRICES_H

#ifndef AVD_MATH_USE_SIMD
#include "math/avd_matrix_non_simd.h"
#else 
#error "AVD_MATH_USE_SIMD is not supported in this version."
#endif

#define avdMatTranslation(tx, ty, tz) avdMat4x4( \
    1.0f, 0.0f, 0.0f, tx, \
    0.0f, 1.0f, 0.0f, ty, \
    0.0f, 0.0f, 1.0f, tz, \
    0.0f, 0.0f, 0.0f, 1.0f \
)

#define avdMatRotationX(angle) avdMat4x4( \
    1.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, avdCos(angle), -avdSin(angle), 0.0f, \
    0.0f, avdSin(angle), avdCos(angle), 0.0f, \
    0.0f, 0.0f, 0.0f, 1.0f \
)

#define avdMatRotationY(angle) avdMat4x4( \
    avdCos(angle), 0.0f, avdSin(angle), 0.0f, \
    0.0f, 1.0f, 0.0f, 0.0f, \
    -avdSin(angle), 0.0f, avdCos(angle), 0.0f, \
    0.0f, 0.0f, 0.0f, 1.0f \
)

#define avdMatRotationZ(angle) avdMat4x4( \
    avdCos(angle), -avdSin(angle), 0.0f, 0.0f, \
    avdSin(angle), avdCos(angle), 0.0f, 0.0f, \
    0.0f, 0.0f, 1.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 1.0f \
)

#define avdMatScale(sx, sy, sz) avdMat4x4( \
    sx, 0.0f, 0.0f, 0.0f, \
    0.0f, sy, 0.0f, 0.0f, \
    0.0f, 0.0f, sz, 0.0f, \
    0.0f, 0.0f, 0.0f, 1.0f \
)

#define avdMatLookAt(eye, center, up) ({ \
    AVD_Vector3 f = avdVec3Normalize(avdVec3Subtract(center, eye)); \
    AVD_Vector3 s = avdVec3Normalize(avdVec3Cross(f, up)); \
    AVD_Vector3 u = avdVec3Cross(s, f); \
    avdMat4x4( \
        s.x, u.x, -f.x, 0.0f, \
        s.y, u.y, -f.y, 0.0f, \
        s.z, u.z, -f.z, 0.0f, \
        -avdVec3Dot(s, eye), -avdVec3Dot(u, eye), avdVec3Dot(f, eye), 1.0f \
    ); \
})

#define avdMatPerspective(fovY, aspect, nearZ, farZ) ({ \
    AVD_Float f = 1.0f / avdTan(fovY * 0.5f); \
    avdMat4x4( \
        f / aspect, 0.0f, 0.0f, 0.0f, \
        0.0f, f, 0.0f, 0.0f, \
        0.0f, 0.0f, (farZ + nearZ) / (nearZ - farZ), -1.0f, \
        0.0f, 0.0f, (2.0f * farZ * nearZ) / (nearZ - farZ), 0.0f \
    ); \
})

#define avdMatOrthographic(left, right, bottom, top, nearZ, farZ) ({ \
    AVD_Float rl = right - left; \
    AVD_Float tb = top - bottom; \
    AVD_Float fn = farZ - nearZ; \
    avdMat4x4( \
        2.0f / rl, 0.0f, 0.0f, -(right + left) / rl, \
        0.0f, 2.0f / tb, 0.0f, -(top + bottom) / tb, \
        0.0f, 0.0f, -2.0f / fn, -(farZ + nearZ) / fn, \
        0.0f, 0.0f, 0.0f, 1.0f \
    ); \
})

#define avdMatFrustum(left, right, bottom, top, nearZ, farZ) ({ \
    AVD_Float rl = right - left; \
    AVD_Float tb = top - bottom; \
    AVD_Float fn = farZ - nearZ; \
    avdMat4x4( \
        (2.0f * nearZ) / rl, 0.0f, (right + left) / rl, 0.0f, \
        0.0f, (2.0f * nearZ) / tb, (top + bottom) / tb, 0.0f, \
        0.0f, 0.0f, -(farZ + nearZ) / fn, -(2.0f * farZ * nearZ) / fn, \
        0.0f, 0.0f, -1.0f, 0.0f \
    ); \
})



#endif // AVD_3D_MATRICES_H