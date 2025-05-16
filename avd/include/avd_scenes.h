#ifndef AVD_SCENES_H
#define AVD_SCENES_H

#include "avd_asset.h"
#include "avd_common.h"
#include "avd_vulkan.h"
#include "avd_font_renderer.h"
#include "avd_bloom.h"


bool avdScenesInit(AVD_GameState *gameState);
void avdScenesShutdown(AVD_GameState *gameState);
bool avdScenesUpdate(AVD_GameState *gameState);
bool avdScenesRender(AVD_GameState *gameState);
bool avdScenesSwitch(AVD_GameState *gameState, AVD_SceneType sceneType);
bool avdScenesSwitchWithoutTransition(AVD_GameState *gameState, AVD_SceneType sceneType);
bool avdScenesLoadContentScenesAsyncPoll(AVD_GameState *gameState);

bool avdScenesLoadingInit(AVD_GameState *gameState);
void avdScenesLoadingShutdown(AVD_GameState *gameState);
bool avdScenesLoadingSwitch(AVD_GameState *gameState);
bool avdScenesLoadingRender(AVD_GameState *gameState);
bool avdScenesLoadingUpdate(AVD_GameState *gameState);

bool avdScenesSplashInit(AVD_GameState *gameState);
void avdScenesSplashShutdown(AVD_GameState *gameState);
bool avdScenesSplashSwitch(AVD_GameState *gameState);
bool avdScenesSplashRender(AVD_GameState *gameState);
bool avdScenesSplashUpdate(AVD_GameState *gameState);

bool avdScenesMainMenuInit(AVD_GameState *gameState);
void avdScenesMainMenuShutdown(AVD_GameState *gameState);
bool avdScenesMainMenuSwitch(AVD_GameState *gameState);
bool avdScenesMainMenuUpdate(AVD_GameState *gameState);
bool avdScenesMainMenuRender(AVD_GameState *gameState);

bool avdScenesPrologueInit(AVD_GameState *gameState);
void avdScenesPrologueShutdown(AVD_GameState *gameState);
bool avdScenesPrologueSwitch(AVD_GameState *gameState);
bool avdScenesPrologueUpdate(AVD_GameState *gameState);
bool avdScenesPrologueRender(AVD_GameState *gameState);

#endif // AVD_SCENES_H