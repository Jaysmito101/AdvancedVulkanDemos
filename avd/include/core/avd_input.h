#ifndef AVD_INPUT_H
#define AVD_INPUT_H

struct AVD_Window;

typedef struct AVD_Input {
    bool keyState[1024];
    bool mouseButtonState[1024];

    float lastMouseX;
    float lastMouseY;

    double rawMouseX;
    double rawMouseY;

    float mouseX;
    float mouseY;

    float mouseDeltaX;
    float mouseDeltaY;

    float mouseScrollX; 
    float mouseScrollY;
} AVD_Input;

typedef enum AVD_InputEventType {
    AVD_INPUT_EVENT_NONE,
    AVD_INPUT_EVENT_KEY,
    AVD_INPUT_EVENT_MOUSE_BUTTON,
    AVD_INPUT_EVENT_MOUSE_MOVE,
    AVD_INPUT_EVENT_MOUSE_SCROLL,
    AVD_INPUT_EVENT_WINDOW_MOVE,
    AVD_INPUT_EVENT_WINDOW_RESIZE,
    AVD_INPUT_EVENT_WINDOW_CLOSE,
    AVD_INPUT_EVENT_DRAG_N_DROP,
} AVD_InputEventType;

typedef struct AVD_InputEventKey {
    AVD_InputEventType type;
    int key;
    int scancode;
    int action;
    int mods;
} AVD_InputEventKey;

typedef struct AVD_InputEventMouseButton {
    AVD_InputEventType type;
    int button;
    int action;
    int mods;
} AVD_InputEventMouseButton;

typedef struct AVD_InputEventMouseMove {
    AVD_InputEventType type;
    float x;
    float y;
} AVD_InputEventMouseMove;

typedef struct AVD_InputEventMouseScroll {
    AVD_InputEventType type;
    float x;
    float y;
} AVD_InputEventMouseScroll;

typedef struct AVD_InputEventWindowMove {
    AVD_InputEventType type;
    int x;
    int y;
} AVD_InputEventWindowMove;

typedef struct AVD_InputEventWindowResize {
    AVD_InputEventType type;
    int width;
    int height;
} AVD_InputEventWindowResize;

typedef struct AVD_InputEventWindowClose {
    AVD_InputEventType type;
} AVD_InputEventWindowClose;

typedef struct AVD_InputEventDragNDrop {
    AVD_InputEventType type;
    int count;
    const char **paths;
} AVD_InputEventDragNDrop;

typedef union AVD_InputEvent {
    AVD_InputEventType type;
    AVD_InputEventKey key;
    AVD_InputEventMouseButton mouseButton;
    AVD_InputEventMouseMove mouseMove;
    AVD_InputEventMouseScroll mouseScroll;
    AVD_InputEventWindowMove windowMove;
    AVD_InputEventWindowResize windowResize;
    AVD_InputEventWindowClose windowClose;
    AVD_InputEventDragNDrop dragNDrop;
} AVD_InputEvent;

const char* avdInputEventTypeToString(AVD_InputEventType type);
void avdInputCalculateMousePositionFromRaw(AVD_Input *input, struct AVD_Window* window, double x, double y);
void avdInputCalculateDeltas(AVD_Input *input);
void avdInputNewFrame(AVD_Input *input);

#endif // AVD_INPUT_H