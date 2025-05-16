#include "avd_application.h"

void avdInputCalculateMousePositionFromRaw(AVD_GameState *gameState, double x, double y)
{
    AVD_ASSERT(gameState != NULL);

    gameState->input.rawMouseX = x;
    gameState->input.rawMouseY = y;

    float width = (float)gameState->window.width;
    float height = (float)gameState->window.height;
    float aspectRatio = width / height;
    float gameAspectRatio = (float)GAME_WIDTH / (float)GAME_HEIGHT;

    float mx = (float)x / width;
    float my = 1.0f - (float)y / height;

    mx = mx * 2.0f - 1.0f;
    my = my * 2.0f - 1.0f;

    if (aspectRatio > gameAspectRatio)
    {
        mx *= aspectRatio / gameAspectRatio;
    }
    else
    {
        my *= gameAspectRatio / aspectRatio;
    }

    gameState->input.mouseX = mx;
    gameState->input.mouseY = my;
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