#ifndef AVD_MATRIX_NON_SIMD_H
#define AVD_MATRIX_NON_SIMD_H

#include "math/avd_vector_non_simd.h"

// Important Note:
// This file defines matrix types and operations without SIMD optimizations.
// The matrices are laid out in memory in column-major order,
// however you must only access them through the provided macros and functions.
// and thus can use normal row-major access patterns.

typedef union {
    AVD_Float m[4 * 4];
    struct {
        AVD_Vector4 cols[4];
    };
    struct {
        AVD_Vector4 col0;
        AVD_Vector4 col1;
        AVD_Vector4 col2;
        AVD_Vector4 col3;
    };
} AVD_Matrix4x4;

typedef union {
    AVD_Float m[3 * 3];
    struct {
        AVD_Vector3 cols[3];
    };
    struct {
        AVD_Vector3 col0;
        AVD_Vector3 col1;
        AVD_Vector3 col2;
    };
} AVD_Matrix3x3;

#define avdMat4x4( \
    r0c0, r0c1, r0c2, r0c3, \
    r1c0, r1c1, r1c2, r1c3, \
    r2c0, r2c1, r2c2, r2c3, \
    r3c0, r3c1, r3c2, r3c3 \
) ((AVD_Matrix4x4){ \
    .m = { \
        r0c0, r1c0, r2c0, r3c0, \
        r0c1, r1c1, r2c1, r3c1, \
        r0c2, r1c2, r2c2, r3c2, \
        r0c3, r1c3, r2c3, r3c3 \
    } \
})
#define avdMat4x4Val(mat, row, col) ((mat).m[(col) * 4 + (row)])
#define avdMat4x4Row(mat, row) avdVec4( \
    avdMat4x4Val(mat, row, 0), \
    avdMat4x4Val(mat, row, 1), \
    avdMat4x4Val(mat, row, 2), \
    avdMat4x4Val(mat, row, 3) \
)
#define avdMat4x4Col(mat, col) avdVec4( \
    avdMat4x4Val(mat, 0, col), \
    avdMat4x4Val(mat, 1, col), \
    avdMat4x4Val(mat, 2, col), \
    avdMat4x4Val(mat, 3, col) \
)
#define avdMat4x4Diagonal(val) avdMat4x4( \
    (val), 0.0f, 0.0f, 0.0f, \
    0.0f, (val), 0.0f, 0.0f, \
    0.0f, 0.0f, (val), 0.0f, \
    0.0f, 0.0f, 0.0f, (val) \
)
#define avdMat4x4Zero() avdMat4x4Diagonal(0.0f)
#define avdMat4x4Identity() avdMat4x4Diagonal(1.0f)
#define avdMat4x4FromCols(c0, c1, c2, c3) ((AVD_Matrix4x4){ .col0 = (c0), .col1 = (c1), .col2 = (c2), .col3 = (c3) })
#define avdMat4x4FromRows(r0, r1, r2, r3) avdMat4x4( \
    (r0).x, (r1).x, (r2).x, (r3).x, \
    (r0).y, (r1).y, (r2).y, (r3).y, \
    (r0).z, (r1).z, (r2).z, (r3).z, \
    (r0).w, (r1).w, (r2).w, (r3).w  \
)
#define avdMat4x4FromMat3x3(mat) avdMat4x4( \
    (mat).col0.x, (mat).col1.x, (mat).col2.x, 0.0f, \
    (mat).col0.y, (mat).col1.y, (mat).col2.y, 0.0f, \
    (mat).col0.z, (mat).col1.z, (mat).col2.z, 0.0f, \
    0.0f, 0.0f, 0.0f, 1.0f \
)
#define avdMat4x4Transpose(mat) avdMat4x4( \
    (mat).col0.x, (mat).col0.y, (mat).col0.z, (mat).col0.w, \
    (mat).col1.x, (mat).col1.y, (mat).col1.z, (mat).col1.w, \
    (mat).col2.x, (mat).col2.y, (mat).col2.z, (mat).col2.w, \
    (mat).col3.x, (mat).col3.y, (mat).col3.z, (mat).col3.w  \
)
#define avdMat4x4Log(mat) AVD_LOG("Matrix4x4[ \n" \
            "  %.2f, %.2f, %.2f, %.2f,\n" \
            "  %.2f, %.2f, %.2f, %.2f,\n" \
            "  %.2f, %.2f, %.2f, %.2f,\n" \
            "  %.2f, %.2f, %.2f, %.2f\n" \
            "]", \
            (mat).col0.x, (mat).col1.x, (mat).col2.x, (mat).col3.x, \
            (mat).col0.y, (mat).col1.y, (mat).col2.y, (mat).col3.y, \
            (mat).col0.z, (mat).col1.z, (mat).col2.z, (mat).col3.z, \
            (mat).col0.w, (mat).col1.w, (mat).col2.w, (mat).col3.w)


#define avdMat3x3( \
    r0c0, r0c1, r0c2, \
    r1c0, r1c1, r1c2, \
    r2c0, r2c1, r2c2 \
) ((AVD_Matrix3x3){ \
    .m = { \
        r0c0, r1c0, r2c0, \
        r0c1, r1c1, r2c1, \
        r0c2, r1c2, r2c2 \
    } \
})
#define avdMat3x3Val(mat, row, col) ((mat).m[(col) * 3 + (row)])
#define avdMat3x3Row(mat, row) avdVec3( \
    avdMat3x3Val(mat, row, 0), \
    avdMat3x3Val(mat, row, 1), \
    avdMat3x3Val(mat, row, 2) \
)
#define avdMat3x3Col(mat, col) avdVec3( \
    avdMat3x3Val(mat, 0, col), \
    avdMat3x3Val(mat, 1, col), \
    avdMat3x3Val(mat, 2, col) \
)
#define avdMat3x3Diagonal(val) avdMat3x3( \
    (val), 0.0f, 0.0f, \
    0.0f, (val), 0.0f, \
    0.0f, 0.0f, (val) \
)
#define avdMat3x3Zero() avdMat3x3Diagonal(0.0f)
#define avdMat3x3Identity() avdMat3x3Diagonal(1.0f)
#define avdMat3x3FromCols(c0, c1, c2) ((AVD_Matrix3x3){ .col0 = (c0), .col1 = (c1), .col2 = (c2) })
#define avdMat3x3FromRows(r0, r1, r2) avdMat3x3( \
    (r0).x, (r1).x, (r2).x, \
    (r0).y, (r1).y, (r2).y, \
    (r0).z, (r1).z, (r2).z  \
)
#define avdMat3x3Transpose(mat) avdMat3x3( \
    (mat).col0.x, (mat).col0.y, (mat).col0.z, \
    (mat).col1.x, (mat).col1.y, (mat).col1.z, \
    (mat).col2.x, (mat).col2.y, (mat).col2.z  \
)
#define avdMat3x3FromMat4x4(mat) avdMat3x3( \
    (mat).col0.x, (mat).col1.x, (mat).col2.x, \
    (mat).col0.y, (mat).col1.y, (mat).col2.y, \
    (mat).col0.z, (mat).col1.z, (mat).col2.z  \
)
#define avdMat3x3Log(mat) AVD_LOG("Matrix3x3[ \n" \
            "  %.2f, %.2f, %.2f,\n" \
            "  %.2f, %.2f, %.2f,\n" \
            "  %.2f, %.2f, %.2f\n" \
            "]", \
            (mat).col0.x, (mat).col1.x, (mat).col2.x, \
            (mat).col0.y, (mat).col1.y, (mat).col2.y, \
            (mat).col0.z, (mat).col1.z, (mat).col2.z)
            



void avdMat4x4AddInplace(AVD_Matrix4x4* out, const AVD_Matrix4x4* b);
AVD_Matrix4x4 avdMat4x4Add(const AVD_Matrix4x4 a, const AVD_Matrix4x4 b);
void avdMat4x4SubtractInplace(AVD_Matrix4x4* out, const AVD_Matrix4x4* a);
AVD_Matrix4x4 avdMat4x4Subtract(const AVD_Matrix4x4 a, const AVD_Matrix4x4 b);
void avdMat4x4ScaleInplace(AVD_Matrix4x4* out, const AVD_Matrix4x4* a, AVD_Float scalar);
AVD_Matrix4x4 avdMat4x4Scale(const AVD_Matrix4x4 a, AVD_Float scalar);
void avdMat4x4MultiplyInplace(AVD_Matrix4x4* out, const AVD_Matrix4x4* b);
AVD_Matrix4x4 avdMat4x4Multiply(const AVD_Matrix4x4 a, const AVD_Matrix4x4 b);
AVD_Vector4 avdMat4x4MultiplyVec4(const AVD_Matrix4x4 mat, const AVD_Vector4 vec);







#endif // AVD_MATRIX_NON_SIMD_H