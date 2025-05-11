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
    case PS_SCENE_TYPE_LOADING:
        psScenesLoadingSwitch(gameState);
        break;
    case PS_SCENE_TYPE_MAIN_MENU:
        psScenesMainMenuSwitch(gameState);
        break;
    case PS_SCENE_TYPE_PROLOGUE:
        psScenesPrologueSwitch(gameState);
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
    gameState->scene.sceneSwitchDuration = 0.0; // Initialize duration

    if (!psScenesSplashInit(gameState))
    {
        PS_LOG("Failed to initialize splash scene\n");
        return false;
    }

    if (!psScenesLoadingInit(gameState)) // Added loading scene init
    {
        PS_LOG("Failed to initialize loading scene\n");
        return false;
    }

    memset(gameState->scene.contentScenesLoaded, 0, sizeof(gameState->scene.contentScenesLoaded));
    gameState->scene.allContentScenesLoaded = false;
    gameState->scene.loadingProgress = 0.0f;

    return true;
}

void psScenesShutdown(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    psScenesSplashShutdown(gameState);
    psScenesLoadingShutdown(gameState);
    psScenesMainMenuShutdown(gameState);
    // psScenesPrologueShutdown(gameState);
}


bool psScenesUpdate(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    static PS_SceneType sceneToUpdate = PS_SCENE_TYPE_NONE;
    const double midPointDuration = gameState->scene.sceneSwitchDuration / 2.0;

    if (gameState->scene.isSwitchingScene)
    {
        double elapsedTime = gameState->framerate.currentTime - gameState->scene.sceneSwitchStartTime;
        if (elapsedTime < midPointDuration)
        {
            double progress = elapsedTime / midPointDuration;
            gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS * __psSceneChageDriverCurve(1.0f - (float)progress);
            sceneToUpdate = gameState->scene.previousScene;
        }
        else if (elapsedTime < gameState->scene.sceneSwitchDuration)
        {
            if (sceneToUpdate != gameState->scene.currentScene) // Check if switch already happened
            {
                __psSwitchScene(gameState, gameState->scene.currentScene);
                sceneToUpdate = gameState->scene.currentScene; // Update sceneToUpdate after switch
            }

            double progress = (elapsedTime - midPointDuration) / midPointDuration;
            gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS * __psSceneChageDriverCurve((float)progress);
        }
        else
        {
            if (sceneToUpdate != gameState->scene.currentScene) // Check if switch already happened
            {
                __psSwitchScene(gameState, gameState->scene.currentScene);
                sceneToUpdate = gameState->scene.currentScene; // Update sceneToUpdate after switch
            }

            gameState->scene.isSwitchingScene = false;
            gameState->scene.previousScene = PS_SCENE_TYPE_NONE;
            gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS;
        }
    }
    else {
		sceneToUpdate = gameState->scene.currentScene;
    }

    switch (sceneToUpdate)
    {
    case PS_SCENE_TYPE_SPLASH:
        return psScenesSplashUpdate(gameState);
    case PS_SCENE_TYPE_LOADING:
        return psScenesLoadingUpdate(gameState);
    case PS_SCENE_TYPE_MAIN_MENU:
        return psScenesMainMenuUpdate(gameState);
    case PS_SCENE_TYPE_PROLOGUE:
        return psScenesPrologueUpdate(gameState);
    case PS_SCENE_TYPE_NONE:
        return true;
    default:
        PS_LOG("Unknown scene type for update: %d\n", sceneToUpdate);
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
    else
    {
        // Ensure the presentation circle is fully open if not transitioning
        gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS;
    }

    switch (sceneToRender)
    {
    case PS_SCENE_TYPE_SPLASH:
        return psScenesSplashRender(gameState);
    case PS_SCENE_TYPE_LOADING:
        return psScenesLoadingRender(gameState);
    case PS_SCENE_TYPE_MAIN_MENU:
        return psScenesMainMenuRender(gameState);
    case PS_SCENE_TYPE_PROLOGUE:
        return psScenesPrologueRender(gameState);
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

    if (gameState->scene.isSwitchingScene)
    {
        PS_LOG("Warning: Attempted to switch scene while already switching.\n");
        return false;
    }

    gameState->scene.previousScene = gameState->scene.currentScene;
    gameState->scene.currentScene = sceneType;
    gameState->scene.isSwitchingScene = true;
    gameState->scene.sceneSwitchStartTime = gameState->framerate.currentTime;
    gameState->scene.sceneSwitchDuration = PS_SCENE_CHANGE_DURATION;
    gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS;

    return true;
}

bool psScenesSwitchWithoutTransition(PS_GameState *gameState, PS_SceneType sceneType)
{
    PS_ASSERT(gameState != NULL);

    gameState->scene.previousScene = PS_SCENE_TYPE_NONE;
    gameState->scene.currentScene = sceneType;
    gameState->scene.isSwitchingScene = false;
    gameState->scene.sceneSwitchStartTime = 0.0;
    gameState->scene.sceneSwitchDuration = 0.0;
    gameState->vulkan.renderer.presentation.circleRadius = PS_SCENE_CHANGE_MAX_CIRCLE_RADIUS;
    __psSwitchScene(gameState, sceneType);
    return true;
}

#define PS_LOAD_CONTENT_SCENE(index, name, code)  \
    if (!gameState->scene.contentScenesLoaded[index]) {  \
        if(!code) {  \
            PS_LOG("Failed to load content scene %d [%s]\n", index, name);  \
            return false;  \
        } \
        gameState->scene.contentScenesLoaded[index] = true;  \
        gameState->scene.loadingProgress = (float)(index + 1) / PS_TOTAL_SCENES;  \
        PS_LOG("Loaded content scene %d [%s]\n", index, name);  \
        if (gameState->scene.allContentScenesLoaded) {  \
            PS_LOG("All content scenes loaded\n");  \
        }  \
        return true; \
    }


bool fakeLoad() {
    _sleep(100);
    return true;
}

// NOTE: This is a crap way to do this, but I dont wanna spend too much time on it building a proper asset loader/manager
bool psScenesLoadContentScenesAsyncPoll(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    PS_LOAD_CONTENT_SCENE(0, "Main Menu", psScenesMainMenuInit(gameState));
    // PS_LOAD_CONTENT_SCENE(1, "Prologue", psScenesPrologueInit(gameState));
    PS_LOAD_CONTENT_SCENE(2, "Dummy 2", fakeLoad());
    PS_LOAD_CONTENT_SCENE(3, "Dummy 4", fakeLoad());
    PS_LOAD_CONTENT_SCENE(4, "Dummy 5", fakeLoad());
    PS_LOAD_CONTENT_SCENE(5, "Dummy 6", fakeLoad());

    gameState->scene.allContentScenesLoaded = true;
    return true;
}