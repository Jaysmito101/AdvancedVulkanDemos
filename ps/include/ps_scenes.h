#ifndef PS_SCENES_H
#define PS_SCENES_H

#include "ps_common.h"
#include "ps_vulkan.h"

bool psScenesInit(PS_GameState *gameState);
void psScenesShutdown(PS_GameState *gameState);
bool psScenesUpdate(PS_GameState *gameState);
bool psScenesRender(PS_GameState *gameState);
bool psScenesSwitch(PS_GameState *gameState, PS_SceneType sceneType);
bool psScenesSwitchWithoutTransition(PS_GameState *gameState, PS_SceneType sceneType);

// Loading scene function declarations
bool psScenesLoadingInit(PS_GameState *gameState);
void psScenesLoadingShutdown(PS_GameState *gameState);
bool psScenesLoadingSwitch(PS_GameState *gameState);
bool psScenesLoadingRender(PS_GameState *gameState);
bool psScenesLoadingUpdate(PS_GameState *gameState);

bool psScenesSplashInit(PS_GameState *gameState);
void psScenesSplashShutdown(PS_GameState *gameState);
bool psScenesSplashSwitch(PS_GameState *gameState);
bool psScenesSplashRender(PS_GameState *gameState);
bool psScenesSplashUpdate(PS_GameState *gameState);

#endif // PS_SCENES_H