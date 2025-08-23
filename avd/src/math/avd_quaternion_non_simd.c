#include "math/avd_qaternion_non_simd.h"
#include "math/avd_matrix_non_simd.h"

#ifndef AVD_USE_SIMD

AVD_Quaternion avdQuatNormalize(const AVD_Quaternion q)
{
    AVD_Float length = avdQuatLength(q);
    if (length > 0.0f) {
        return avdQuatScale(q, 1.0f / length);
    }
    return avdQuatIdentity();
}

AVD_Quaternion avdQuatMultiply(const AVD_Quaternion a, const AVD_Quaternion b)
{
    return avdQuat(
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    );
}

AVD_Quaternion avdQuatInverse(const AVD_Quaternion q)
{
    AVD_Float lengthSq = avdQuatLengthSq(q);
    if (lengthSq > 0.0f) {
        AVD_Quaternion conjugate = avdQuatConjugate(q);
        return avdQuatScale(conjugate, 1.0f / lengthSq);
    }
    return avdQuatIdentity();
}

AVD_Vector3 avdQuatRotateVec3(const AVD_Quaternion q, const AVD_Vector3 v)
{
    // Using the optimized formula: v' = v + 2 * q.xyz × (q.xyz × v + q.w * v)
    AVD_Vector3 qVec = avdVec3(q.x, q.y, q.z);
    AVD_Vector3 cross1 = avdVec3Cross(qVec, v);
    AVD_Vector3 cross2 = avdVec3Cross(qVec, cross1);
    
    return avdVec3Add(v, avdVec3Add(avdVec3Scale(cross1, 2.0f * q.w), avdVec3Scale(cross2, 2.0f)));
}

AVD_Quaternion avdQuatSlerp(const AVD_Quaternion a, const AVD_Quaternion b, AVD_Float t)
{
    AVD_Float dot = avdQuatDot(a, b);
    
    // If the dot product is negative, slerp won't take the shorter path.
    // Note that we should not negate the quaternion if the dot product is negative,
    // but instead negate one of the quaternions to ensure we take the shorter path.
    AVD_Quaternion b_adjusted = b;
    if (dot < 0.0f) {
        b_adjusted = avdQuatScale(b, -1.0f);
        dot = -dot;
    }
    
    // If the quaternions are very close, use linear interpolation
    if (dot > 0.9995f) {
        AVD_Quaternion result = avdQuatAdd(a, avdQuatScale(avdQuatSubtract(b_adjusted, a), t));
        return avdQuatNormalize(result);
    }
    
    AVD_Float theta = acosf(avdClamp(dot, -1.0f, 1.0f));
    AVD_Float sinTheta = avdSin(theta);
    AVD_Float factor1 = avdSin((1.0f - t) * theta) / sinTheta;
    AVD_Float factor2 = avdSin(t * theta) / sinTheta;
    
    return avdQuatAdd(avdQuatScale(a, factor1), avdQuatScale(b_adjusted, factor2));
}

AVD_Quaternion avdQuatFromEulerAngles(AVD_Float pitch, AVD_Float yaw, AVD_Float roll)
{
    // Convert to radians and half angles
    AVD_Float halfPitch = pitch * 0.5f;
    AVD_Float halfYaw = yaw * 0.5f;
    AVD_Float halfRoll = roll * 0.5f;
    
    AVD_Float cosPitch = avdCos(halfPitch);
    AVD_Float sinPitch = avdSin(halfPitch);
    AVD_Float cosYaw = avdCos(halfYaw);
    AVD_Float sinYaw = avdSin(halfYaw);
    AVD_Float cosRoll = avdCos(halfRoll);
    AVD_Float sinRoll = avdSin(halfRoll);
    
    return avdQuat(
        sinPitch * cosYaw * cosRoll + cosPitch * sinYaw * sinRoll,
        cosPitch * sinYaw * cosRoll - sinPitch * cosYaw * sinRoll,
        cosPitch * cosYaw * sinRoll - sinPitch * sinYaw * cosRoll,
        cosPitch * cosYaw * cosRoll + sinPitch * sinYaw * sinRoll
    );
}

AVD_Vector3 avdQuatToEulerAngles(const AVD_Quaternion q)
{
    AVD_Vector3 angles;
    
    // Roll (x-axis rotation)
    AVD_Float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    AVD_Float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    angles.x = atan2f(sinr_cosp, cosr_cosp);
    
    // Pitch (y-axis rotation)
    AVD_Float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (avdAbs(sinp) >= 1.0f) {
        angles.y = copysignf(AVD_PI / 2.0f, sinp); // Use 90 degrees if out of range
    } else {
        angles.y = asinf(sinp);
    }
    
    // Yaw (z-axis rotation)
    AVD_Float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    AVD_Float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    angles.z = atan2f(siny_cosp, cosy_cosp);
    
    return angles;
}

AVD_Matrix4x4 avdQuatToMatrix4x4(const AVD_Quaternion q)
{
    AVD_Float x = q.x;
    AVD_Float y = q.y;
    AVD_Float z = q.z;
    AVD_Float w = q.w;
    
    AVD_Float x2 = x + x;
    AVD_Float y2 = y + y;
    AVD_Float z2 = z + z;
    
    AVD_Float xx = x * x2;
    AVD_Float xy = x * y2;
    AVD_Float xz = x * z2;
    AVD_Float yy = y * y2;
    AVD_Float yz = y * z2;
    AVD_Float zz = z * z2;
    AVD_Float wx = w * x2;
    AVD_Float wy = w * y2;
    AVD_Float wz = w * z2;
    
    return avdMat4x4(
        1.0f - (yy + zz), xy - wz,         xz + wy,         0.0f,
        xy + wz,         1.0f - (xx + zz), yz - wx,         0.0f,
        xz - wy,         yz + wx,         1.0f - (xx + yy), 0.0f,
        0.0f,            0.0f,            0.0f,            1.0f
    );
}

AVD_Matrix3x3 avdQuatToMatrix3x3(const AVD_Quaternion q)
{
   AVD_Matrix4x4 mat = avdQuatToMatrix4x4(q);
   return avdMat3x3FromMat4x4(mat);
}

AVD_Quaternion avdQuatFromMatrix4x4(const AVD_Matrix4x4 mat)
{
    AVD_Float trace = avdMat4x4Val(mat, 0, 0) + avdMat4x4Val(mat, 1, 1) + avdMat4x4Val(mat, 2, 2);
    
    if (trace > 0.0f) {
        AVD_Float s = avdSqrt(trace + 1.0f) * 2.0f; // s = 4 * qw
        return avdQuat(
            (avdMat4x4Val(mat, 2, 1) - avdMat4x4Val(mat, 1, 2)) / s,
            (avdMat4x4Val(mat, 0, 2) - avdMat4x4Val(mat, 2, 0)) / s,
            (avdMat4x4Val(mat, 1, 0) - avdMat4x4Val(mat, 0, 1)) / s,
            0.25f * s
        );
    } else if ((avdMat4x4Val(mat, 0, 0) > avdMat4x4Val(mat, 1, 1)) && (avdMat4x4Val(mat, 0, 0) > avdMat4x4Val(mat, 2, 2))) {
        AVD_Float s = avdSqrt(1.0f + avdMat4x4Val(mat, 0, 0) - avdMat4x4Val(mat, 1, 1) - avdMat4x4Val(mat, 2, 2)) * 2.0f; // s = 4 * qx
        return avdQuat(
            0.25f * s,
            (avdMat4x4Val(mat, 0, 1) + avdMat4x4Val(mat, 1, 0)) / s,
            (avdMat4x4Val(mat, 0, 2) + avdMat4x4Val(mat, 2, 0)) / s,
            (avdMat4x4Val(mat, 2, 1) - avdMat4x4Val(mat, 1, 2)) / s
        );
    } else if (avdMat4x4Val(mat, 1, 1) > avdMat4x4Val(mat, 2, 2)) {
        AVD_Float s = avdSqrt(1.0f + avdMat4x4Val(mat, 1, 1) - avdMat4x4Val(mat, 0, 0) - avdMat4x4Val(mat, 2, 2)) * 2.0f; // s = 4 * qy
        return avdQuat(
            (avdMat4x4Val(mat, 0, 1) + avdMat4x4Val(mat, 1, 0)) / s,
            0.25f * s,
            (avdMat4x4Val(mat, 1, 2) + avdMat4x4Val(mat, 2, 1)) / s,
            (avdMat4x4Val(mat, 0, 2) - avdMat4x4Val(mat, 2, 0)) / s
        );
    } else {
        AVD_Float s = avdSqrt(1.0f + avdMat4x4Val(mat, 2, 2) - avdMat4x4Val(mat, 0, 0) - avdMat4x4Val(mat, 1, 1)) * 2.0f; // s = 4 * qz
        return avdQuat(
            (avdMat4x4Val(mat, 0, 2) + avdMat4x4Val(mat, 2, 0)) / s,
            (avdMat4x4Val(mat, 1, 2) + avdMat4x4Val(mat, 2, 1)) / s,
            0.25f * s,
            (avdMat4x4Val(mat, 1, 0) - avdMat4x4Val(mat, 0, 1)) / s
        );
    }
}

AVD_Quaternion avdQuatFromMatrix3x3(const AVD_Matrix3x3 mat)
{
    return avdQuatFromMatrix4x4(avdMat4x4FromMat3x3(mat));
}

AVD_Quaternion avdQuatLookAt(const AVD_Vector3 forward, const AVD_Vector3 up)
{
    AVD_Vector3 f = avdVec3Normalize(forward);
    AVD_Vector3 u = avdVec3Normalize(up);
    AVD_Vector3 r = avdVec3Normalize(avdVec3Cross(u, f));
    u = avdVec3Cross(f, r);
    
    // Create rotation matrix
    AVD_Matrix3x3 rotMat = avdMat3x3FromRows(r, u, f);
    
    return avdQuatFromMatrix3x3(rotMat);
}

#endif
