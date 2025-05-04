#ifndef PS_APPLICATION_H
#define PS_APPLICATION_H


#include "ps_common.h"
#include "ps_scenes.h"

bool psApplicationInit(PS_GameState *gameState);
void psApplicationShutdown(PS_GameState *gameState);

bool psApplicationIsRunning(PS_GameState *gameState);
void psApplicationUpdate(PS_GameState *gameState);
void psApplicationUpdateWithoutPolling(PS_GameState *gameState);
void psApplicationRender(PS_GameState *gameState);

void psInputCalculateMousePositionFromRaw(PS_GameState *gameState, double x, double y);

#endif // PS_APPLICATION_H