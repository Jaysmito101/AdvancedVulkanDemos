#ifndef AVD_FPS_CAMERA_H
#define AVD_FPS_CAMERA_H

#include "core/avd_core.h"
#include "core/avd_input.h"
#include "core/avd_types.h"
#include "math/avd_math.h"

typedef struct {
    AVD_Vector3 cameraPosition;
    AVD_Float cameraYaw;
    AVD_Float cameraPitch;
    AVD_Float cameraRoll;

    AVD_Vector2 pitchLimits;
    AVD_Vector2 yawLimits;
    AVD_Vector2 rollLimits;

    AVD_Float rotateSensitivity;
    AVD_Float moveSpeed;

    AVD_Bool freezeAxis[3];
    AVD_Bool invertAxis[3];
    
    AVD_Vector3 cameraDirection;
    AVD_Matrix4x4 matrix;
} AVD_FpsCamera;

void avdFpsCameraInit(AVD_FpsCamera *camera);
void avdFpsCameraSetPosition(AVD_FpsCamera *camera, AVD_Vector3 position);
void avdFpsCameraSetYaw(AVD_FpsCamera *camera, AVD_Float yaw);
void avdFpsCameraSetPitch(AVD_FpsCamera *camera, AVD_Float pitch);
void avdFpsCameraSetRoll(AVD_FpsCamera *camera, AVD_Float roll);
void avdFpsCameraLookAt(AVD_FpsCamera *camera, AVD_Vector3 target, AVD_Vector3 up);
void avdFpsCameraMoveForward(AVD_FpsCamera *camera, AVD_Float distance);
void avdFpsCameraMoveRight(AVD_FpsCamera *camera, AVD_Float distance);
void avdFpsCameraMoveUp(AVD_FpsCamera *camera, AVD_Float distance);
void avdFpsCameraOnInputEvent(AVD_FpsCamera *camera, AVD_InputEvent *event, AVD_Input *input);
void avdFpsCameraUpdate(AVD_FpsCamera *camera, AVD_Input *input, AVD_Float deltaTime);

#endif // AVD_FPS_CAMERA_H
