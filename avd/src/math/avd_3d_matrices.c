#include "math/avd_3d_matrices.h"

AVD_Matrix4x4 avdMatLookAt(const AVD_Vector3 eye, const AVD_Vector3 center, const AVD_Vector3 up)
{
    AVD_Vector3 f = avdVec3Normalize(avdVec3Subtract(center, eye));
    AVD_Vector3 s = avdVec3Normalize(avdVec3Cross(f, up));
    AVD_Vector3 u = avdVec3Cross(s, f);

    return avdMat4x4(
        s.x, u.x, -f.x, 0.0f,
        s.y, u.y, -f.y, 0.0f,
        s.z, u.z, -f.z, 0.0f,
        -avdVec3Dot(s, eye), -avdVec3Dot(u, eye), avdVec3Dot(f, eye), 1.0f);
}

AVD_Matrix4x4 avdMatPerspective(AVD_Float fovY, AVD_Float aspect, AVD_Float nearZ, AVD_Float farZ)
{
    AVD_Float f        = 1.0f / avdTan(fovY * 0.5f);
    AVD_Float rangeInv = 1.0f / (nearZ - farZ);

    return avdMat4x4(
        f / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, (farZ + nearZ) * rangeInv, -1.0f,
        0.0f, 0.0f, farZ * nearZ * rangeInv * 2.0f, 0.0f);
}

AVD_Matrix4x4 avdMatOrthographic(AVD_Float left, AVD_Float right, AVD_Float bottom, AVD_Float top, AVD_Float nearZ, AVD_Float farZ)
{
    AVD_Float width  = right - left;
    AVD_Float height = top - bottom;
    AVD_Float depth  = farZ - nearZ;

    return avdMat4x4(
        2.0f / width, 0.0f, 0.0f, -(right + left) / width,
        0.0f, 2.0f / height, 0.0f, -(top + bottom) / height,
        0.0f, 0.0f, -2.0f / depth, -(farZ + nearZ) / depth,
        0.0f, 0.0f, 0.0f, 1.0f);
}

AVD_Matrix4x4 avdMatFrustum(AVD_Float left, AVD_Float right, AVD_Float bottom, AVD_Float top, AVD_Float nearZ, AVD_Float farZ)
{
    AVD_Float width  = right - left;
    AVD_Float height = top - bottom;
    AVD_Float depth  = farZ - nearZ;

    return avdMat4x4(
        2.0f * nearZ / width, 0.0f, (right + left) / width, 0.0f,
        0.0f, 2.0f * nearZ / height, (top + bottom) / height, 0.0f,
        0.0f, 0.0f, -(farZ + nearZ) / depth, -2.0f * farZ * nearZ / depth,
        0.0f, 0.0f, -1.0f, 0.0f);
}
