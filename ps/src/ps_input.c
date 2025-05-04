#include "ps_application.h"

void psInputCalculateMousePositionFromRaw(PS_GameState *gameState, double x, double y) {
    PS_ASSERT(gameState != NULL);

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
    
    if (aspectRatio > gameAspectRatio) {
        mx *= aspectRatio / gameAspectRatio;
    } else {
        my *= gameAspectRatio / aspectRatio;
    }

    gameState->input.mouseX = mx;
    gameState->input.mouseY = my;    
}