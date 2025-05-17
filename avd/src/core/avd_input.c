#include "core/avd_input.h"
#include "core/avd_window.h"

void avdInputCalculateMousePositionFromRaw(AVD_Input *input, AVD_Window *window, double x, double y)
{
    AVD_ASSERT(input != NULL);
    AVD_ASSERT(window != NULL);

    input->rawMouseX = x;
    input->rawMouseY = y;

    float width           = (float)window->width;
    float height          = (float)window->height;
    float aspectRatio     = width / height;
    float gameAspectRatio = (float)GAME_WIDTH / (float)GAME_HEIGHT;

    float mx = (float)x / width;
    float my = 1.0f - (float)y / height;

    mx = mx * 2.0f - 1.0f;
    my = my * 2.0f - 1.0f;

    if (aspectRatio > gameAspectRatio) {
        mx *= aspectRatio / gameAspectRatio;
    } else {
        my *= gameAspectRatio / aspectRatio;
    }

    input->mouseX = mx;
    input->mouseY = my;
}

void avdInputCalculateDeltas(AVD_Input *input)
{
    AVD_ASSERT(input != NULL);

    input->mouseDeltaX = input->mouseX - input->lastMouseX;
    input->mouseDeltaY = input->mouseY - input->lastMouseY;
}

void avdInputNewFrame(AVD_Input *input)
{
    AVD_ASSERT(input != NULL);

    input->mouseDeltaX = 0.0f;
    input->mouseDeltaY = 0.0f;

    input->mouseScrollX = 0.0f;
    input->mouseScrollY = 0.0f;

    input->lastMouseX = input->mouseX;
    input->lastMouseY = input->mouseY;
}

const char *avdInputEventTypeToString(AVD_InputEventType type)
{
    switch (type) {
        case AVD_INPUT_EVENT_NONE:
            return "InputEvent_None";
        case AVD_INPUT_EVENT_KEY:
            return "InputEvent_Key";
        case AVD_INPUT_EVENT_MOUSE_BUTTON:
            return "InputEvent_MouseButton";
        case AVD_INPUT_EVENT_MOUSE_MOVE:
            return "InputEvent_MouseMove";
        case AVD_INPUT_EVENT_MOUSE_SCROLL:
            return "InputEvent_MouseScroll";
        case AVD_INPUT_EVENT_WINDOW_MOVE:
            return "InputEvent_WindowMove";
        case AVD_INPUT_EVENT_WINDOW_RESIZE:
            return "InputEvent_WindowResize";
        case AVD_INPUT_EVENT_WINDOW_CLOSE:
            return "InputEvent_WindowClose";
        case AVD_INPUT_EVENT_DRAG_N_DROP:
            return "InputEvent_DragNDrop";
        default:
            return "InputEvent_Unknown";
    }

    return "InputEvent_Unknown";
}