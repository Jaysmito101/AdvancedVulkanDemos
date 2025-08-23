#ifndef AVD_QUATERNION_NON_SIMD_H
#define AVD_QUATERNION_NON_SIMD_H

#include "math/avd_vector_non_simd.h"
#include "math/avd_matrix_non_simd.h"

typedef union {
    AVD_Float q[4];
    struct {
        AVD_Float x;
        AVD_Float y;
        AVD_Float z;
        AVD_Float w;
    };
    struct {
        AVD_Float i;
        AVD_Float j;
        AVD_Float k;
        AVD_Float r;
    };
    struct {
        AVD_Vector3 vec;
        AVD_Float ww;
    };
} AVD_Quaternion;

#define avdQuat(qx, qy, qz, qw)           ((AVD_Quaternion){.x = (qx), .y = (qy), .z = (qz), .w = (qw)})
#define avdQuatIdentity()                 avdQuat(0.0f, 0.0f, 0.0f, 1.0f)
#define avdQuatZero()                     avdQuat(0.0f, 0.0f, 0.0f, 0.0f)
#define avdQuatAdd(a, b)                  avdQuat((a).x + (b).x, (a).y + (b).y, (a).z + (b).z, (a).w + (b).w)
#define avdQuatSubtract(a, b)             avdQuat((a).x - (b).x, (a).y - (b).y, (a).z - (b).z, (a).w - (b).w)
#define avdQuatScale(q, scalar)           avdQuat((q).x * (scalar), (q).y * (scalar), (q).z * (scalar), (q).w * (scalar))
#define avdQuatLengthSq(q)                ((q).x * (q).x + (q).y * (q).y + (q).z * (q).z + (q).w * (q).w)
#define avdQuatLength(q)                  (avdSqrt(avdQuatLengthSq(q)))
#define avdQuatInvLength(q)               (avdSqrt(avdQuatLengthSq(q)) > 0.0f ? 1.0f / avdSqrt(avdQuatLengthSq(q)) : 0.0f)
#define avdQuatDot(q1, q2)                ((q1).x * (q2).x + (q1).y * (q2).y + (q1).z * (q2).z + (q1).w * (q2).w)
#define avdQuatConjugate(q)               avdQuat(-(q).x, -(q).y, -(q).z, (q).w)
#define avdQuatFromAxisAngle(axis, angle) avdQuat( \
    (axis).x * avdSin((angle) * 0.5f),            \
    (axis).y * avdSin((angle) * 0.5f),            \
    (axis).z * avdSin((angle) * 0.5f),            \
    avdCos((angle) * 0.5f))
#define avdQuatFromEuler(pitch, yaw, roll) avdQuatFromEulerAngles((pitch), (yaw), (roll))
#define avdQuatLog(q)                     AVD_LOG("Quat[x: %.2f, y: %.2f, z: %.2f, w: %.2f]", (q).x, (q).y, (q).z, (q).w)

AVD_Quaternion avdQuatNormalize(const AVD_Quaternion q);
AVD_Quaternion avdQuatMultiply(const AVD_Quaternion a, const AVD_Quaternion b);
AVD_Quaternion avdQuatInverse(const AVD_Quaternion q);
AVD_Vector3 avdQuatRotateVec3(const AVD_Quaternion q, const AVD_Vector3 v);
AVD_Quaternion avdQuatSlerp(const AVD_Quaternion a, const AVD_Quaternion b, AVD_Float t);
AVD_Quaternion avdQuatFromEulerAngles(AVD_Float pitch, AVD_Float yaw, AVD_Float roll);
AVD_Vector3 avdQuatToEulerAngles(const AVD_Quaternion q);
AVD_Matrix4x4 avdQuatToMatrix4x4(const AVD_Quaternion q);
AVD_Matrix3x3 avdQuatToMatrix3x3(const AVD_Quaternion q);
AVD_Quaternion avdQuatFromMatrix4x4(const AVD_Matrix4x4 mat);
AVD_Quaternion avdQuatFromMatrix3x3(const AVD_Matrix3x3 mat);
AVD_Quaternion avdQuatLookAt(const AVD_Vector3 forward, const AVD_Vector3 up);

#endif // AVD_QUATERNION_NON_SIMD_H