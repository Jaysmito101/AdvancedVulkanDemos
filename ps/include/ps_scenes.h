#ifndef PS_SCENES_H
#define PS_SCENES_H

#include "ps_asset.h"
#include "ps_common.h"
#include "ps_vulkan.h"
#include "ps_font_renderer.h"
#include "ps_bloom.h"


bool psScenesInit(PS_GameState *gameState);
void psScenesShutdown(PS_GameState *gameState);
bool psScenesUpdate(PS_GameState *gameState);
bool psScenesRender(PS_GameState *gameState);
bool psScenesSwitch(PS_GameState *gameState, PS_SceneType sceneType);
bool psScenesSwitchWithoutTransition(PS_GameState *gameState, PS_SceneType sceneType);
bool psScenesLoadContentScenesAsyncPoll(PS_GameState *gameState);

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

bool psScenesMainMenuInit(PS_GameState *gameState);
void psScenesMainMenuShutdown(PS_GameState *gameState);
bool psScenesMainMenuSwitch(PS_GameState *gameState);
bool psScenesMainMenuUpdate(PS_GameState *gameState);
bool psScenesMainMenuRender(PS_GameState *gameState);

bool psScenesPrologueInit(PS_GameState *gameState);
void psScenesPrologueShutdown(PS_GameState *gameState);
bool psScenesPrologueSwitch(PS_GameState *gameState);
bool psScenesPrologueUpdate(PS_GameState *gameState);
bool psScenesPrologueRender(PS_GameState *gameState);

#endif // PS_SCENES_H