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


void avdInputCalculateMousePositionFromRaw(AVD_Input *input, struct AVD_Window* window, double x, double y);
void avdInputCalculateDeltas(AVD_Input *input);
void avdInputNewFrame(AVD_Input *input);

#endif // AVD_INPUT_H