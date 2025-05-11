#ifndef PS_WINDOW_H
#define PS_WINDOW_H


#include "ps_common.h"

bool psWindowInit(PS_GameState *gameState);
void psWindowShutdown(PS_Window *window);
void psWindowPollEvents();


#endif // PS_WINDOW_H