#ifndef AVD_WINDOW_H
#define AVD_WINDOW_H


#include "avd_common.h"

bool avdWindowInit(AVD_Window* window, AVD_GameState *gameState);
void avdWindowShutdown(AVD_Window *window);
void avdWindowPollEvents();


#endif // AVD_WINDOW_H