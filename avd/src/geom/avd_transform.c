#include "geom/avd_transform.h"

AVD_Matrix4x4 avdTransformToMatrix(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    
    // Create translation matrix
    AVD_Matrix4x4 translation = avdMatTranslation(
        transform->position.x, 
        transform->position.y, 
        transform->position.z
    );
    
    // Create rotation matrix from quaternion
    AVD_Matrix4x4 rotation = avdQuatToMatrix4x4(transform->rotation);
    
    // Create scale matrix
    AVD_Matrix4x4 scale = avdMatScale(
        transform->scale.x, 
        transform->scale.y, 
        transform->scale.z
    );
    
    // Combine transformations: Translation * Rotation * Scale
    AVD_Matrix4x4 result = avdMat4x4Multiply(rotation, scale);
    result = avdMat4x4Multiply(translation, result);
    
    return result;
}

AVD_Transform avdTransformFromMatrix(const AVD_Matrix4x4 *matrix)
{
    AVD_ASSERT(matrix != NULL);
    
    AVD_Transform transform;
    
    // Extract position (translation) from the last column
    transform.position = avdVec3(
        avdMat4x4Val(*matrix, 0, 3),
        avdMat4x4Val(*matrix, 1, 3),
        avdMat4x4Val(*matrix, 2, 3)
    );
    
    // Extract scale from the length of the first three columns
    transform.scale = avdMatGetScale(matrix);
    
    // Remove scale to get pure rotation matrix
    AVD_Matrix4x4 rotationMatrix = avdMatRemoveScale(matrix);
    
    // Convert rotation matrix to quaternion
    transform.rotation = avdQuatFromMatrix4x4(rotationMatrix);
    
    return transform;
}

AVD_Transform avdTransformCombine(const AVD_Transform *parent, const AVD_Transform *child)
{
    AVD_ASSERT(parent != NULL && child != NULL);
    
    AVD_Transform result;
    
    // Combine rotations
    result.rotation = avdQuatMultiply(parent->rotation, child->rotation);
    
    // Combine scales
    result.scale = avdVec3(
        parent->scale.x * child->scale.x,
        parent->scale.y * child->scale.y,
        parent->scale.z * child->scale.z
    );
    
    // Transform child position by parent rotation and scale, then add parent position
    AVD_Vector3 scaledChildPos = avdVec3(
        child->position.x * parent->scale.x,
        child->position.y * parent->scale.y,
        child->position.z * parent->scale.z
    );
    AVD_Vector3 rotatedChildPos = avdQuatRotateVec3(parent->rotation, scaledChildPos);
    result.position = avdVec3Add(parent->position, rotatedChildPos);
    
    return result;
}

AVD_Transform avdTransformInverse(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    
    AVD_Transform inverse;
    
    // Inverse rotation
    inverse.rotation = avdQuatConjugate(transform->rotation);
    
    // Inverse scale
    inverse.scale = avdVec3(
        transform->scale.x != 0.0f ? 1.0f / transform->scale.x : 0.0f,
        transform->scale.y != 0.0f ? 1.0f / transform->scale.y : 0.0f,
        transform->scale.z != 0.0f ? 1.0f / transform->scale.z : 0.0f
    );
    
    // Inverse translation: first inverse rotate, then inverse scale
    AVD_Vector3 inverseRotatedPos = avdQuatRotateVec3(inverse.rotation, avdVec3Scale(transform->position, -1.0f));
    inverse.position = avdVec3(
        inverseRotatedPos.x * inverse.scale.x,
        inverseRotatedPos.y * inverse.scale.y,
        inverseRotatedPos.z * inverse.scale.z
    );
    
    return inverse;
}

AVD_Vector3 avdTransformPoint(const AVD_Transform *transform, const AVD_Vector3 point)
{
    AVD_ASSERT(transform != NULL);
    
    // Scale, then rotate, then translate
    AVD_Vector3 scaled = avdVec3(
        point.x * transform->scale.x,
        point.y * transform->scale.y,
        point.z * transform->scale.z
    );
    
    AVD_Vector3 rotated = avdQuatRotateVec3(transform->rotation, scaled);
    return avdVec3Add(rotated, transform->position);
}

AVD_Vector3 avdTransformDirection(const AVD_Transform *transform, const AVD_Vector3 direction)
{
    AVD_ASSERT(transform != NULL);
    
    // Only apply rotation to direction vectors (no translation or uniform scaling)
    return avdQuatRotateVec3(transform->rotation, direction);
}

AVD_Transform avdTransformLerp(const AVD_Transform *a, const AVD_Transform *b, AVD_Float t)
{
    AVD_ASSERT(a != NULL && b != NULL);
    
    AVD_Transform result;
    
    // Interpolate position
    result.position = avdVec3Lerp(a->position, b->position, t);
    
    // Interpolate scale
    result.scale = avdVec3Lerp(a->scale, b->scale, t);
    
    // Spherical interpolation for rotation
    result.rotation = avdQuatSlerp(a->rotation, b->rotation, t);
    
    return result;
}

// Component setters
void avdTransformSetPosition(AVD_Transform *transform, const AVD_Vector3 position)
{
    AVD_ASSERT(transform != NULL);
    transform->position = position;
}

void avdTransformSetRotation(AVD_Transform *transform, const AVD_Quaternion rotation)
{
    AVD_ASSERT(transform != NULL);
    transform->rotation = avdQuatNormalize(rotation);
}

void avdTransformSetScale(AVD_Transform *transform, const AVD_Vector3 scale)
{
    AVD_ASSERT(transform != NULL);
    transform->scale = scale;
}

void avdTransformSetEulerAngles(AVD_Transform *transform, const AVD_Vector3 eulerAngles)
{
    AVD_ASSERT(transform != NULL);
    transform->rotation = avdQuatFromEulerAngles(eulerAngles.x, eulerAngles.y, eulerAngles.z);
}

// Component getters
AVD_Vector3 avdTransformGetPosition(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    return transform->position;
}

AVD_Quaternion avdTransformGetRotation(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    return transform->rotation;
}

AVD_Vector3 avdTransformGetScale(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    return transform->scale;
}

AVD_Vector3 avdTransformGetEulerAngles(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    return avdQuatToEulerAngles(transform->rotation);
}

// Transform modification
void avdTransformTranslate(AVD_Transform *transform, const AVD_Vector3 translation)
{
    AVD_ASSERT(transform != NULL);
    transform->position = avdVec3Add(transform->position, translation);
}

void avdTransformRotate(AVD_Transform *transform, const AVD_Quaternion rotation)
{
    AVD_ASSERT(transform != NULL);
    transform->rotation = avdQuatNormalize(avdQuatMultiply(transform->rotation, rotation));
}

void avdTransformRotateEuler(AVD_Transform *transform, const AVD_Vector3 eulerAngles)
{
    AVD_ASSERT(transform != NULL);
    AVD_Quaternion deltaRotation = avdQuatFromEulerAngles(eulerAngles.x, eulerAngles.y, eulerAngles.z);
    avdTransformRotate(transform, deltaRotation);
}

void avdTransformScaleBy(AVD_Transform *transform, const AVD_Vector3 scale)
{
    AVD_ASSERT(transform != NULL);
    transform->scale = avdVec3(
        transform->scale.x * scale.x,
        transform->scale.y * scale.y,
        transform->scale.z * scale.z
    );
}

// Transform lookAt functionality
void avdTransformLookAt(AVD_Transform *transform, const AVD_Vector3 target, const AVD_Vector3 up)
{
    AVD_ASSERT(transform != NULL);
    
    AVD_Vector3 forward = avdVec3Normalize(avdVec3Subtract(target, transform->position));
    transform->rotation = avdQuatLookAt(forward, up);
}

AVD_Vector3 avdTransformGetForward(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    // Forward is negative Z in our coordinate system
    return avdQuatRotateVec3(transform->rotation, avdVec3(0.0f, 0.0f, -1.0f));
}

AVD_Vector3 avdTransformGetUp(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    // Up is positive Y
    return avdQuatRotateVec3(transform->rotation, avdVec3(0.0f, 1.0f, 0.0f));
}

AVD_Vector3 avdTransformGetRight(const AVD_Transform *transform)
{
    AVD_ASSERT(transform != NULL);
    // Right is positive X
    return avdQuatRotateVec3(transform->rotation, avdVec3(1.0f, 0.0f, 0.0f));
}
