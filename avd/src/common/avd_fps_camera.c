#include "common/avd_fps_camera.h"
#include "geom/avd_3d_matrices.h"

static void PRIV_avdFpsCameraUpdate(AVD_FpsCamera *camera)
{
    AVD_ASSERT(camera != NULL);

    camera->cameraYaw   = AVD_CLAMP(camera->cameraYaw, camera->yawLimits.x, camera->yawLimits.y);
    camera->cameraPitch = AVD_CLAMP(camera->cameraPitch, camera->pitchLimits.x, camera->pitchLimits.y);
    camera->cameraRoll  = AVD_CLAMP(camera->cameraRoll, camera->rollLimits.x, camera->rollLimits.y);

    camera->cameraDirection = avdVec3(
        cosf(camera->cameraPitch) * sinf(camera->cameraYaw),
        sinf(camera->cameraPitch),
        cosf(camera->cameraPitch) * cosf(camera->cameraYaw));
    camera->cameraDirection = avdVec3Normalize(camera->cameraDirection);

    AVD_Vector3 right = avdVec3Normalize(avdVec3Cross(camera->cameraDirection, avdVec3(0.0f, 1.0f, 0.0f)));
    AVD_Vector3 up    = avdVec3Normalize(avdVec3Cross(right, camera->cameraDirection));
    if (camera->cameraRoll != 0.0f) {
        AVD_Float cosRoll = cosf(camera->cameraRoll);
        AVD_Float sinRoll = sinf(camera->cameraRoll);

        AVD_Vector3 rolledRight = avdVec3Add(avdVec3Scale(right, cosRoll), avdVec3Scale(up, -sinRoll));
        AVD_Vector3 rolledUp    = avdVec3Add(avdVec3Scale(right, sinRoll), avdVec3Scale(up, cosRoll));

        right = rolledRight;
        up    = rolledUp;
    }
    camera->matrix = avdMatLookAt(camera->cameraPosition, avdVec3Add(camera->cameraPosition, camera->cameraDirection), up);
}

void avdFpsCameraInit(AVD_FpsCamera *camera)
{
    AVD_ASSERT(camera != NULL);

    memset(camera, 0, sizeof(AVD_FpsCamera));
    camera->cameraDirection = avdVec3(0.0f, 0.0f, -1.0f);
    camera->cameraPosition  = avdVec3(0.0f, 0.0f, 0.0f);
    camera->cameraYaw       = 0.0f;
    camera->cameraPitch     = 0.0f;
    camera->cameraRoll      = 0.0f;
    camera->matrix          = avdMat4x4Identity();

    camera->pitchLimits = avdVec2(-AVD_PI * 0.45f, AVD_PI * 0.45f);
    camera->yawLimits   = avdVec2(-AVD_PI * 10.0f, AVD_PI * 10.0f);
    camera->rollLimits  = avdVec2(-AVD_PI, AVD_PI);

    camera->rotateSensitivity = 0.6f;
    camera->moveSpeed         = 0.5f;

    camera->freezeAxis[0] = false;
    camera->freezeAxis[1] = false;
    camera->freezeAxis[2] = false;
    camera->invertAxis[0] = false;
    camera->invertAxis[1] = false;
    camera->invertAxis[2] = false;

    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraSetPosition(AVD_FpsCamera *camera, AVD_Vector3 position)
{
    AVD_ASSERT(camera != NULL);
    if (!camera->freezeAxis[0])
        camera->cameraPosition.x = position.x;
    if (!camera->freezeAxis[1])
        camera->cameraPosition.y = position.y;
    if (!camera->freezeAxis[2])
        camera->cameraPosition.z = position.z;
    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraSetYaw(AVD_FpsCamera *camera, AVD_Float yaw)
{
    AVD_ASSERT(camera != NULL);
    camera->cameraYaw = yaw;
    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraSetPitch(AVD_FpsCamera *camera, AVD_Float pitch)
{
    AVD_ASSERT(camera != NULL);
    camera->cameraPitch = pitch;
    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraSetRoll(AVD_FpsCamera *camera, AVD_Float roll)
{
    AVD_ASSERT(camera != NULL);
    camera->cameraRoll = roll;
    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraLookAt(AVD_FpsCamera *camera, AVD_Vector3 target, AVD_Vector3 up)
{
    AVD_ASSERT(camera != NULL);

    AVD_Vector3 direction = avdVec3Normalize(avdVec3Subtract(target, camera->cameraPosition));

    camera->cameraPitch = asinf(direction.y);
    camera->cameraYaw   = atan2f(direction.x, direction.z);

    AVD_Vector3 right     = avdVec3Normalize(avdVec3Cross(direction, avdVec3(0.0f, 1.0f, 0.0f)));
    AVD_Vector3 naturalUp = avdVec3Normalize(avdVec3Cross(right, direction));

    AVD_Vector3 upProjected = avdVec3Normalize(avdVec3Subtract(up, avdVec3Scale(direction, avdVec3Dot(up, direction))));

    AVD_Float dotProduct = avdVec3Dot(naturalUp, upProjected);
    AVD_Float crossY     = avdVec3Dot(avdVec3Cross(naturalUp, upProjected), direction);
    camera->cameraRoll   = atan2f(crossY, dotProduct);

    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraMoveForward(AVD_FpsCamera *camera, AVD_Float distance)
{
    AVD_ASSERT(camera != NULL);
    AVD_Vector3 delta = avdVec3Scale(camera->cameraDirection, distance);
    if (!camera->freezeAxis[0])
        camera->cameraPosition.x += delta.x;
    if (!camera->freezeAxis[1])
        camera->cameraPosition.y += delta.y;
    if (!camera->freezeAxis[2])
        camera->cameraPosition.z += delta.z;
    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraMoveRight(AVD_FpsCamera *camera, AVD_Float distance)
{
    AVD_ASSERT(camera != NULL);
    AVD_Vector3 right = avdVec3Normalize(avdVec3Cross(camera->cameraDirection, avdVec3(0.0f, 1.0f, 0.0f)));
    AVD_Vector3 delta = avdVec3Scale(right, distance);
    if (!camera->freezeAxis[0])
        camera->cameraPosition.x += delta.x;
    if (!camera->freezeAxis[1])
        camera->cameraPosition.y += delta.y;
    if (!camera->freezeAxis[2])
        camera->cameraPosition.z += delta.z;
    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraMoveUp(AVD_FpsCamera *camera, AVD_Float distance)
{
    AVD_ASSERT(camera != NULL);
    if (!camera->freezeAxis[1])
        camera->cameraPosition.y += distance;
    PRIV_avdFpsCameraUpdate(camera);
}

void avdFpsCameraOnInputEvent(AVD_FpsCamera *camera, AVD_InputEvent *event, AVD_Input *input)
{
    AVD_ASSERT(camera != NULL);
    AVD_ASSERT(event != NULL);
    AVD_ASSERT(input != NULL);

    AVD_Bool dirty = false;

    if (event->type == AVD_INPUT_EVENT_MOUSE_MOVE) {
        AVD_Float yawMult   = camera->invertAxis[1] ? -1.0f : 1.0f;
        AVD_Float pitchMult = camera->invertAxis[0] ? -1.0f : 1.0f;
        camera->cameraYaw += input->mouseDeltaX * camera->rotateSensitivity * yawMult;
        camera->cameraPitch -= input->mouseDeltaY * camera->rotateSensitivity * pitchMult;
        dirty = true;
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_SCROLL) {
        AVD_Vector3 delta = avdVec3Scale(camera->cameraDirection, event->mouseScroll.y * camera->moveSpeed);
        if (!camera->freezeAxis[0])
            camera->cameraPosition.x += delta.x;
        if (!camera->freezeAxis[1])
            camera->cameraPosition.y += delta.y;
        if (!camera->freezeAxis[2])
            camera->cameraPosition.z += delta.z;
        dirty = true;
    }

    if (dirty) {
        PRIV_avdFpsCameraUpdate(camera);
    }
}

void avdFpsCameraUpdate(AVD_FpsCamera *camera, AVD_Input *input, AVD_Float deltaTime)
{
    AVD_ASSERT(camera != NULL);
    AVD_ASSERT(input != NULL);

    AVD_Vector3 forward = camera->cameraDirection;
    AVD_Vector3 up      = avdVec3(0.0f, 1.0f, 0.0f);
    AVD_Vector3 right   = avdVec3Normalize(avdVec3Cross(forward, up));

    AVD_Bool dirty = false;

    if (camera->moveSpeed > 0.0f) {
        AVD_Vector3 delta = avdVec3(0.0f, 0.0f, 0.0f);
        if (input->keyState[GLFW_KEY_W])
            delta = avdVec3Add(delta, avdVec3Scale(forward, camera->moveSpeed));
        if (input->keyState[GLFW_KEY_S])
            delta = avdVec3Add(delta, avdVec3Scale(forward, -camera->moveSpeed));
        if (input->keyState[GLFW_KEY_A])
            delta = avdVec3Add(delta, avdVec3Scale(right, -camera->moveSpeed));
        if (input->keyState[GLFW_KEY_D])
            delta = avdVec3Add(delta, avdVec3Scale(right, camera->moveSpeed));
        if (input->keyState[GLFW_KEY_Q])
            delta = avdVec3Add(delta, avdVec3Scale(up, camera->moveSpeed));
        if (input->keyState[GLFW_KEY_E])
            delta = avdVec3Add(delta, avdVec3Scale(up, -camera->moveSpeed));
        if (delta.x != 0.0f || delta.y != 0.0f || delta.z != 0.0f) {
            if (!camera->freezeAxis[0])
                camera->cameraPosition.x += delta.x * deltaTime;
            if (!camera->freezeAxis[1])
                camera->cameraPosition.y += delta.y * deltaTime;
            if (!camera->freezeAxis[2])
                camera->cameraPosition.z += delta.z * deltaTime;
            dirty = true;
        }
    }

    if (camera->rotateSensitivity > 0.0f) {
        AVD_Float yawMult   = camera->invertAxis[1] ? -1.0f : 1.0f;
        AVD_Float pitchMult = camera->invertAxis[0] ? -1.0f : 1.0f;
        if (input->keyState[GLFW_KEY_UP]) {
            camera->cameraPitch += camera->rotateSensitivity * pitchMult * deltaTime;
            dirty = true;
        }
        if (input->keyState[GLFW_KEY_LEFT]) {
            camera->cameraYaw += camera->rotateSensitivity * yawMult * deltaTime;
            dirty = true;
        }
        if (input->keyState[GLFW_KEY_DOWN]) {
            camera->cameraPitch -= camera->rotateSensitivity * pitchMult * deltaTime;
            dirty = true;
        }
        if (input->keyState[GLFW_KEY_RIGHT]) {
            camera->cameraYaw -= camera->rotateSensitivity * yawMult * deltaTime;
            dirty = true;
        }
    }

    if (dirty) {
        PRIV_avdFpsCameraUpdate(camera);
    }
}
