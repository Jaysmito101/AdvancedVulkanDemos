#include "ps_application.h"

bool psScenesInit(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

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
    switch (gameState->scene.currentScene)
    {
    case PS_SCENE_TYPE_SPLASH:
        return psScenesSplashUpdate(gameState);
    default:
        PS_LOG("Unknown scene type: %d\n", gameState->scene.currentScene);
        break;
    }
    return false;
}

bool psScenesRender(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    switch (gameState->scene.currentScene)
    {
    case PS_SCENE_TYPE_SPLASH:
        return psScenesSplashRender(gameState);
    default:
        PS_LOG("Unknown scene type: %d\n", gameState->scene.currentScene);
        break;
    }
    return true;
}

bool psScenesSwitch(PS_GameState *gameState, PS_SceneType sceneType)
{
    PS_ASSERT(gameState != NULL);

    switch (sceneType)
    {
    case PS_SCENE_TYPE_SPLASH:
        return psScenesSplashSwitch(gameState);
    default:
        PS_LOG("Unknown scene type: %d\n", sceneType);
        break;
    }
    return false;
}