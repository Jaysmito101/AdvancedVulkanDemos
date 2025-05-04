#include "ps_application.h"
#include <math.h> // For fmaxf

#define PS_SCENE_CHANGE_DURATION 3.0f
#define PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS 3.0f

static void __psSwitchScene(PS_GameState *gameState, PS_SceneType sceneType)
{
    PS_ASSERT(gameState != NULL);

    switch (sceneType)
    {
    case PS_SCENE_TYPE_SPLASH:
        psScenesSplashSwitch(gameState);
        break;
    default:
        PS_LOG("Unknown scene type for switch: %d\n", sceneType);
        break;
    }
}

static float __psSceneChageDriverCurve(float x)
{
    // a cubuic curve for the scene change animation (ease in - ease out)
    return x * x * (3 - 2 * x);
}

bool psScenesInit(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    gameState->scene.previousScene = PS_SCENE_TYPE_NONE;    
    gameState->scene.currentScene = PS_SCENE_TYPE_NONE;
    gameState->scene.isSwitchingScene = false;
    gameState->scene.sceneSwitchStartTime = 0.0;

    if (!psScenesSplashInit(gameState))
    {
        PS_LOG("Failed to initialize splash scene\n");
        return false;
    }

    return true;
}

void psScenesShutdown(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    psScenesSplashShutdown(gameState);
}

bool psScenesUpdate(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    static PS_SceneType lastScene = PS_SCENE_TYPE_NONE;
    const double midPointDuration = gameState->scene.sceneSwitchDuration / 2.0;

    if (gameState->scene.isSwitchingScene)
    {
        double elapsedTime = gameState->framerate.currentTime - gameState->scene.sceneSwitchStartTime;
        if (elapsedTime < midPointDuration)
        {
            double progress = elapsedTime / midPointDuration;
            gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS * __psSceneChageDriverCurve(1.0f - (float)progress);
            lastScene = gameState->scene.previousScene;
        }
        else if (elapsedTime < gameState->scene.sceneSwitchDuration)
        {
            if (lastScene != gameState->scene.currentScene)
            {
                __psSwitchScene(gameState, gameState->scene.currentScene);
            }

            double progress = (elapsedTime - midPointDuration) / midPointDuration;
            gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS * __psSceneChageDriverCurve((float)progress); 
            lastScene = gameState->scene.currentScene;
        }
        else
        {
            if (lastScene != gameState->scene.currentScene)
            {
                __psSwitchScene(gameState, gameState->scene.currentScene);
            }

            gameState->scene.isSwitchingScene = false;
            gameState->scene.previousScene = PS_SCENE_TYPE_NONE;
            gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS;
        }
    }
    else {
		lastScene = gameState->scene.currentScene;
    }

    switch (lastScene)
    {
    case PS_SCENE_TYPE_SPLASH:
        return psScenesSplashUpdate(gameState);
    case PS_SCENE_TYPE_NONE:
        return true;
    default:
        PS_LOG("Unknown scene type for update: %d\n", lastScene);
        break;
    }
    return true;
}

bool psScenesRender(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    PS_SceneType sceneToRender = gameState->scene.currentScene;
    const double midPointDuration = gameState->scene.sceneSwitchDuration / 2.0;

    if (gameState->scene.isSwitchingScene)
    {
        double elapsedTime = gameState->framerate.currentTime - gameState->scene.sceneSwitchStartTime;
        if (elapsedTime < midPointDuration)
        {
            sceneToRender = gameState->scene.previousScene;
        }
    }

    switch (sceneToRender)
    {
    case PS_SCENE_TYPE_SPLASH:
        return psScenesSplashRender(gameState);
    case PS_SCENE_TYPE_NONE:
         return true;
    default:
        PS_LOG("Unknown scene type for render: %d\n", sceneToRender);
        break;
    }
    return false;
}

bool psScenesSwitch(PS_GameState *gameState, PS_SceneType sceneType)
{
    PS_ASSERT(gameState != NULL);

    if (gameState->scene.isSwitchingScene) {
        PS_LOG("Warning: Attempted to switch scene while already switching.\n");
        return false;
    }

    gameState->scene.previousScene = gameState->scene.currentScene;
    gameState->scene.currentScene = sceneType;
    gameState->scene.isSwitchingScene = true;
    gameState->scene.sceneSwitchStartTime = gameState->framerate.currentTime;
    gameState->scene.sceneSwitchDuration = 3.0;

    return true; 
}