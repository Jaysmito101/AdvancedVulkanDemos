#ifndef PS_APPLICATION_H
#define PS_APPLICATION_H


#include "ps_common.h"

bool psApplicationInit(PS_GameState *gameState);
void psApplicationShutdown(PS_GameState *gameState);

bool psApplicationIsRunning(PS_GameState *gameState);
void psApplicationUpdate(PS_GameState *gameState);


#endif // PS_APPLICATION_H