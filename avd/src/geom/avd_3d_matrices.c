#include "geom/avd_3d_matrices.h"

AVD_Matrix4x4 avdMatLookAt(const AVD_Vector3 eye, const AVD_Vector3 center, const AVD_Vector3 up)
{
    AVD_Vector3 f = avdVec3Normalize(avdVec3Subtract(center, eye));
    AVD_Vector3 s = avdVec3Normalize(avdVec3Cross(f, up));
    AVD_Vector3 u = avdVec3Cross(s, f);

    return avdMat4x4(
        s.x, s.y, s.z, -avdVec3Dot(s, eye),
        u.x, u.y, u.z, -avdVec3Dot(u, eye),
        -f.x, -f.y, -f.z, avdVec3Dot(f, eye),
        0.0f, 0.0f, 0.0f, 1.0f);
}

AVD_Matrix4x4 avdMatPerspective(AVD_Float fovY, AVD_Float aspect, AVD_Float nearZ, AVD_Float farZ)
{
    AVD_Float tanHalfFovY = avdTan(fovY * 0.5f);
    return avdMat4x4(
        1.0f / (aspect * tanHalfFovY), 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f / tanHalfFovY, 0.0f, 0.0f,
        0.0f, 0.0f, -(farZ + nearZ) / (farZ - nearZ), -2.0f * farZ * nearZ / (farZ - nearZ),
        0.0f, 0.0f, -1.0f, 0.0f);
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

AVD_Vector3 avdMatGetScale(const AVD_Matrix4x4 *mat)
{
    AVD_ASSERT(mat != NULL);
    return avdVec3(
        avdVec3Length(avdMat4x4Col(*mat, 0)),
        avdVec3Length(avdMat4x4Col(*mat, 1)),
        avdVec3Length(avdMat4x4Col(*mat, 2)));
}

AVD_Matrix4x4 avdMatRemoveScale(const AVD_Matrix4x4 *mat)
{
    AVD_ASSERT(mat != NULL);
    AVD_Matrix4x4 result = *mat;
    result.col0          = avdVec4FromVec3(avdVec3Normalize(avdVecSwizzle3(mat->col0, x, y, z)), 0.0f);
    result.col1          = avdVec4FromVec3(avdVec3Normalize(avdVecSwizzle3(mat->col1, x, y, z)), 0.0f);
    result.col2          = avdVec4FromVec3(avdVec3Normalize(avdVecSwizzle3(mat->col2, x, y, z)), 0.0f);
    return result;
}

AVD_Matrix4x4 avdMatCalculateTransform(
    const AVD_Vector3 position,
    const AVD_Vector3 rotation,
    const AVD_Vector3 scale)
{
    AVD_Matrix4x4 translation = avdMatTranslation(position.x, position.y, position.z);
    AVD_Matrix4x4 rotationX   = avdMatRotationX(rotation.x);
    AVD_Matrix4x4 rotationY   = avdMatRotationY(rotation.y);
    AVD_Matrix4x4 rotationZ   = avdMatRotationZ(rotation.z);
    AVD_Matrix4x4 scaling     = avdMatScale(scale.x, scale.y, scale.z);

    AVD_Matrix4x4 rotationMatrix = avdMat4x4Multiply(
        avdMat4x4Multiply(rotationX, rotationY),
        rotationZ);

    AVD_Matrix4x4 transform = avdMat4x4Multiply(
        translation,
        avdMat4x4Multiply(rotationMatrix, scaling));

    return transform;
}