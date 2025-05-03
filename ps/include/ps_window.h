#ifndef PS_WINDOW_H
#define PS_WINDOW_H


#include "ps_common.h"

bool psWindowInit(PS_GameState *gameState);
void psWindowShutdown(PS_GameState *gameState);
void psWindowPollEvents(PS_GameState *gameState);


#endif // PS_WINDOW_H