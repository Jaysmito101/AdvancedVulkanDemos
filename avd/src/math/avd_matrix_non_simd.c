#include "math/avd_matrix_non_simd.h"

#ifndef AVD_USE_SIMD

void avdMat4x4AddInplace(AVD_Matrix4x4 *out, const AVD_Matrix4x4 *b)
{
    AVD_ASSERT(out != NULL && b != NULL);
    for (int i = 0; i < 16; ++i) {
        out->m[i] += b->m[i];
    }
}

AVD_Matrix4x4 avdMat4x4Add(const AVD_Matrix4x4 a, const AVD_Matrix4x4 b)
{
    AVD_Matrix4x4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = a.m[i] + b.m[i];
    }
    return result;
}

void avdMat4x4SubtractInplace(AVD_Matrix4x4 *out, const AVD_Matrix4x4 *a)
{
    AVD_ASSERT(out != NULL && a != NULL);
    for (int i = 0; i < 16; ++i) {
        out->m[i] -= a->m[i];
    }
}

AVD_Matrix4x4 avdMat4x4Subtract(const AVD_Matrix4x4 a, const AVD_Matrix4x4 b)
{
    AVD_Matrix4x4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = a.m[i] - b.m[i];
    }
    return result;
}

void avdMat4x4ScaleInplace(AVD_Matrix4x4 *out, const AVD_Matrix4x4 *a, AVD_Float scalar)
{
    AVD_ASSERT(out != NULL && a != NULL);
    for (int i = 0; i < 16; ++i) {
        out->m[i] = a->m[i] * scalar;
    }
}

AVD_Matrix4x4 avdMat4x4Scale(const AVD_Matrix4x4 a, AVD_Float scalar)
{
    AVD_Matrix4x4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = a.m[i] * scalar;
    }
    return result;
}

void avdMat4x4MultiplyInplace(AVD_Matrix4x4 *out, const AVD_Matrix4x4 *b)
{
    AVD_ASSERT(out != NULL && b != NULL);
    AVD_Matrix4x4 temp = *out;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            avdMat4x4Val(*out, i, j) = avdMat4x4Val(temp, i, 0) * avdMat4x4Val(*b, 0, j) +
                                       avdMat4x4Val(temp, i, 1) * avdMat4x4Val(*b, 1, j) +
                                       avdMat4x4Val(temp, i, 2) * avdMat4x4Val(*b, 2, j) +
                                       avdMat4x4Val(temp, i, 3) * avdMat4x4Val(*b, 3, j);
        }
    }
}

AVD_Matrix4x4 avdMat4x4Multiply(const AVD_Matrix4x4 a, const AVD_Matrix4x4 b)
{
    AVD_Matrix4x4 result = a;
    avdMat4x4MultiplyInplace(&result, &b);
    return result;
}

AVD_Vector4 avdMat4x4MultiplyVec4(const AVD_Matrix4x4 mat, const AVD_Vector4 vec)
{
    AVD_Vector4 result;
    for (int i = 0; i < 4; ++i) {
        result.v[i] = avdMat4x4Val(mat, i, 0) * vec.x +
                      avdMat4x4Val(mat, i, 1) * vec.y +
                      avdMat4x4Val(mat, i, 2) * vec.z +
                      avdMat4x4Val(mat, i, 3) * vec.w;
    }
    return result;
}

#endif