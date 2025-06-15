#ifndef AVD_3D_MATRICES_H
#define AVD_3D_MATRICES_H

#ifndef AVD_MATH_USE_SIMD
#include "math/avd_matrix_non_simd.h"
#else
#error "AVD_MATH_USE_SIMD is not supported in this version."
#endif

#define avdMatTranslation(tx, ty, tz) avdMat4x4( \
    1.0f, 0.0f, 0.0f, tx,                        \
    0.0f, 1.0f, 0.0f, ty,                        \
    0.0f, 0.0f, 1.0f, tz,                        \
    0.0f, 0.0f, 0.0f, 1.0f)

#define avdMatRotationX(angle) avdMat4x4(      \
    1.0f, 0.0f, 0.0f, 0.0f,                    \
    0.0f, avdCos(angle), -avdSin(angle), 0.0f, \
    0.0f, avdSin(angle), avdCos(angle), 0.0f,  \
    0.0f, 0.0f, 0.0f, 1.0f)

#define avdMatRotationY(angle) avdMat4x4(      \
    avdCos(angle), 0.0f, avdSin(angle), 0.0f,  \
    0.0f, 1.0f, 0.0f, 0.0f,                    \
    -avdSin(angle), 0.0f, avdCos(angle), 0.0f, \
    0.0f, 0.0f, 0.0f, 1.0f)

#define avdMatRotationZ(angle) avdMat4x4(      \
    avdCos(angle), -avdSin(angle), 0.0f, 0.0f, \
    avdSin(angle), avdCos(angle), 0.0f, 0.0f,  \
    0.0f, 0.0f, 1.0f, 0.0f,                    \
    0.0f, 0.0f, 0.0f, 1.0f)

#define avdMatScale(sx, sy, sz) avdMat4x4( \
    sx, 0.0f, 0.0f, 0.0f,                  \
    0.0f, sy, 0.0f, 0.0f,                  \
    0.0f, 0.0f, sz, 0.0f,                  \
    0.0f, 0.0f, 0.0f, 1.0f)

AVD_Matrix4x4 avdMatLookAt(const AVD_Vector3 eye, const AVD_Vector3 center, const AVD_Vector3 up);
AVD_Matrix4x4 avdMatPerspective(AVD_Float fovY, AVD_Float aspect, AVD_Float nearZ, AVD_Float farZ);
AVD_Matrix4x4 avdMatOrthographic(AVD_Float left, AVD_Float right, AVD_Float bottom, AVD_Float top, AVD_Float nearZ, AVD_Float farZ);
AVD_Matrix4x4 avdMatFrustum(AVD_Float left, AVD_Float right, AVD_Float bottom, AVD_Float top, AVD_Float nearZ, AVD_Float farZ);

#endif // AVD_3D_MATRICES_H