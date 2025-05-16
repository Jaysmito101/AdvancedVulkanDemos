#ifndef AVD_APPLICATION_H
#define AVD_APPLICATION_H


#include "avd_common.h"
#include "avd_scenes.h"

bool avdApplicationInit(AVD_GameState *gameState);
void avdApplicationShutdown(AVD_GameState *gameState);

bool avdApplicationIsRunning(AVD_GameState *gameState);
void avdApplicationUpdate(AVD_GameState *gameState);
void avdApplicationUpdateWithoutPolling(AVD_GameState *gameState);
void avdApplicationRender(AVD_GameState *gameState);

void avdInputCalculateMousePositionFromRaw(AVD_GameState *gameState, double x, double y);
void avdInputCalculateDeltas(AVD_Input *input);
void avdInputNewFrame(AVD_Input *input);

#endif // AVD_APPLICATION_H