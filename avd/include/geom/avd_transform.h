#ifndef AVD_TRANSFORM_H
#define AVD_TRANSFORM_H

#include "geom/avd_3d_matrices.h"
#include "math/avd_math.h"

typedef struct {
    AVD_Vector3 position;
    AVD_Vector3 scale;
    AVD_Quaternion rotation;
} AVD_Transform;

// Transform creation and initialization
#define avdTransformIdentity() ((AVD_Transform){ \
    .position = avdVec3Zero(),                   \
    .scale    = avdVec3One(),                    \
    .rotation = avdQuatIdentity()})

#define avdTransform(pos, rot, scl) ((AVD_Transform){ \
    .position = (pos),                                \
    .rotation = (rot),                                \
    .scale    = (scl)})

#define avdTransformLog(transform) AVD_LOG_INFO("Transform[pos: (%.2f, %.2f, %.2f), rot: (%.2f, %.2f, %.2f, %.2f), scale: (%.2f, %.2f, %.2f)]", \
                                                (transform).position.x, (transform).position.y, (transform).position.z,                         \
                                                (transform).rotation.x, (transform).rotation.y, (transform).rotation.z, (transform).rotation.w, \
                                                (transform).scale.x, (transform).scale.y, (transform).scale.z)

// Transform operations
AVD_Matrix4x4 avdTransformToMatrix(const AVD_Transform *transform);
AVD_Transform avdTransformFromMatrix(const AVD_Matrix4x4 *matrix);
AVD_Transform avdTransformCombine(const AVD_Transform *parent, const AVD_Transform *child);
AVD_Transform avdTransformInverse(const AVD_Transform *transform);
AVD_Vector3 avdTransformPoint(const AVD_Transform *transform, const AVD_Vector3 point);
AVD_Vector3 avdTransformDirection(const AVD_Transform *transform, const AVD_Vector3 direction);
AVD_Transform avdTransformLerp(const AVD_Transform *a, const AVD_Transform *b, AVD_Float t);

// Transform component setters
void avdTransformSetPosition(AVD_Transform *transform, const AVD_Vector3 position);
void avdTransformSetRotation(AVD_Transform *transform, const AVD_Quaternion rotation);
void avdTransformSetScale(AVD_Transform *transform, const AVD_Vector3 scale);
void avdTransformSetEulerAngles(AVD_Transform *transform, const AVD_Vector3 eulerAngles);

// Transform component getters
AVD_Vector3 avdTransformGetPosition(const AVD_Transform *transform);
AVD_Quaternion avdTransformGetRotation(const AVD_Transform *transform);
AVD_Vector3 avdTransformGetScale(const AVD_Transform *transform);
AVD_Vector3 avdTransformGetEulerAngles(const AVD_Transform *transform);

// Transform modification
void avdTransformTranslate(AVD_Transform *transform, const AVD_Vector3 translation);
void avdTransformRotate(AVD_Transform *transform, const AVD_Quaternion rotation);
void avdTransformRotateEuler(AVD_Transform *transform, const AVD_Vector3 eulerAngles);
void avdTransformScaleBy(AVD_Transform *transform, const AVD_Vector3 scale);

// Transform lookAt functionality
void avdTransformLookAt(AVD_Transform *transform, const AVD_Vector3 target, const AVD_Vector3 up);
AVD_Vector3 avdTransformGetForward(const AVD_Transform *transform);
AVD_Vector3 avdTransformGetUp(const AVD_Transform *transform);
AVD_Vector3 avdTransformGetRight(const AVD_Transform *transform);

#endif // AVD_TRANSFORM_H