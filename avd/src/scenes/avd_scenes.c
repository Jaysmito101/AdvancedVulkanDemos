#include "avd_application.h"
#include <math.h> // For fmaxf

#define AVD_SCENE_CHANGE_DURATION 3.0f
#define AVD_SCENE_CHANGE_MAX_CIRCLE_RADIUS 3.0f

static void __avdSwitchScene(AVD_GameState *gameState, AVD_SceneType sceneType)
{
    AVD_ASSERT(gameState != NULL);

    switch (sceneType)
    {
    case AVD_SCENE_TYPE_SPLASH:
        avdScenesSplashSwitch(gameState);
        break;
    case AVD_SCENE_TYPE_LOADING:
        avdScenesLoadingSwitch(gameState);
        break;
    case AVD_SCENE_TYPE_MAIN_MENU:
        avdScenesMainMenuSwitch(gameState);
        break;
    case AVD_SCENE_TYPE_PROLOGUE:
        avdScenesPrologueSwitch(gameState);
        break;
    default:
        AVD_LOG("Unknown scene type for switch: %d\n", sceneType);
        break;
    }
}

static float __avdSceneChageDriverCurve(float x)
{
    // a cubuic curve for the scene change animation (ease in - ease out)
    return x * x * (3 - 2 * x);
}

bool avdScenesInit(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);

    gameState->scene.previousScene = AVD_SCENE_TYPE_NONE;
    gameState->scene.currentScene = AVD_SCENE_TYPE_NONE;
    gameState->scene.isSwitchingScene = false;
    gameState->scene.sceneSwitchStartTime = 0.0;
    gameState->scene.sceneSwitchDuration = 0.0; // Initialize duration

    if (!avdScenesSplashInit(gameState))
    {
        AVD_LOG("Failed to initialize splash scene\n");
        return false;
    }

    if (!avdScenesLoadingInit(gameState)) // Added loading scene init
    {
        AVD_LOG("Failed to initialize loading scene\n");
        return false;
    }

    memset(gameState->scene.contentScenesLoaded, 0, sizeof(gameState->scene.contentScenesLoaded));
    gameState->scene.allContentScenesLoaded = false;
    gameState->scene.loadingProgress = 0.0f;

    return true;
}

void avdScenesShutdown(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);

    avdScenesSplashShutdown(gameState);
    avdScenesLoadingShutdown(gameState);
    avdScenesMainMenuShutdown(gameState);
    // avdScenesPrologueShutdown(gameState);
}


bool avdScenesUpdate(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);

    static AVD_SceneType sceneToUpdate = AVD_SCENE_TYPE_NONE;
    const double midPointDuration = gameState->scene.sceneSwitchDuration / 2.0;

    if (gameState->scene.isSwitchingScene)
    {
        double elapsedTime = gameState->framerate.currentTime - gameState->scene.sceneSwitchStartTime;
        if (elapsedTime < midPointDuration)
        {
            double progress = elapsedTime / midPointDuration;
            gameState->vulkan.renderer.presentation.circleRadius = AVD_SCENE_CHANGE_MAX_CIRCLE_RADIUS * __avdSceneChageDriverCurve(1.0f - (float)progress);
            sceneToUpdate = gameState->scene.previousScene;
        }
        else if (elapsedTime < gameState->scene.sceneSwitchDuration)
        {
            if (sceneToUpdate != gameState->scene.currentScene) // Check if switch already happened
            {
                __avdSwitchScene(gameState, gameState->scene.currentScene);
                sceneToUpdate = gameState->scene.currentScene; // Update sceneToUpdate after switch
            }

            double progress = (elapsedTime - midPointDuration) / midPointDuration;
            gameState->vulkan.renderer.presentation.circleRadius = AVD_SCENE_CHANGE_MAX_CIRCLE_RADIUS * __avdSceneChageDriverCurve((float)progress);
        }
        else
        {
            if (sceneToUpdate != gameState->scene.currentScene) // Check if switch already happened
            {
                __avdSwitchScene(gameState, gameState->scene.currentScene);
                sceneToUpdate = gameState->scene.currentScene; // Update sceneToUpdate after switch
            }

            gameState->scene.isSwitchingScene = false;
            gameState->scene.previousScene = AVD_SCENE_TYPE_NONE;
            gameState->vulkan.renderer.presentation.circleRadius = AVD_SCENE_CHANGE_MAX_CIRCLE_RADIUS;
        }
    }
    else {
		sceneToUpdate = gameState->scene.currentScene;
    }

    switch (sceneToUpdate)
    {
    case AVD_SCENE_TYPE_SPLASH:
        return avdScenesSplashUpdate(gameState);
    case AVD_SCENE_TYPE_LOADING:
        return avdScenesLoadingUpdate(gameState);
    case AVD_SCENE_TYPE_MAIN_MENU:
        return avdScenesMainMenuUpdate(gameState);
    case AVD_SCENE_TYPE_PROLOGUE:
        return avdScenesPrologueUpdate(gameState);
    case AVD_SCENE_TYPE_NONE:
        return true;
    default:
        AVD_LOG("Unknown scene type for update: %d\n", sceneToUpdate);
        break;
    }
    return true;
}

bool avdScenesRender(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);

    AVD_SceneType sceneToRender = gameState->scene.currentScene;
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
        gameState->vulkan.renderer.presentation.circleRadius = AVD_SCENE_CHANGE_MAX_CIRCLE_RADIUS;
    }

    switch (sceneToRender)
    {
    case AVD_SCENE_TYPE_SPLASH:
        return avdScenesSplashRender(gameState);
    case AVD_SCENE_TYPE_LOADING:
        return avdScenesLoadingRender(gameState);
    case AVD_SCENE_TYPE_MAIN_MENU:
        return avdScenesMainMenuRender(gameState);
    case AVD_SCENE_TYPE_PROLOGUE:
        return avdScenesPrologueRender(gameState);
    case AVD_SCENE_TYPE_NONE:
        return true;
    default:
        AVD_LOG("Unknown scene type for render: %d\n", sceneToRender);
        break;
    }
    return false;
}

bool avdScenesSwitch(AVD_GameState *gameState, AVD_SceneType sceneType)
{
    AVD_ASSERT(gameState != NULL);

    if (gameState->scene.isSwitchingScene)
    {
        AVD_LOG("Warning: Attempted to switch scene while already switching.\n");
        return false;
    }

    gameState->scene.previousScene = gameState->scene.currentScene;
    gameState->scene.currentScene = sceneType;
    gameState->scene.isSwitchingScene = true;
    gameState->scene.sceneSwitchStartTime = gameState->framerate.currentTime;
    gameState->scene.sceneSwitchDuration = AVD_SCENE_CHANGE_DURATION;
    gameState->vulkan.renderer.presentation.circleRadius = AVD_SCENE_CHANGE_MAX_CIRCLE_RADIUS;

    return true;
}

bool avdScenesSwitchWithoutTransition(AVD_GameState *gameState, AVD_SceneType sceneType)
{
    AVD_ASSERT(gameState != NULL);

    gameState->scene.previousScene = AVD_SCENE_TYPE_NONE;
    gameState->scene.currentScene = sceneType;
    gameState->scene.isSwitchingScene = false;
    gameState->scene.sceneSwitchStartTime = 0.0;
    gameState->scene.sceneSwitchDuration = 0.0;
    gameState->vulkan.renderer.presentation.circleRadius = AVD_SCENE_CHANGE_MAX_CIRCLE_RADIUS;
    __avdSwitchScene(gameState, sceneType);
    return true;
}

#define AVD_LOAD_CONTENT_SCENE(index, name, code)  \
    if (!gameState->scene.contentScenesLoaded[index]) {  \
        if(!code) {  \
            AVD_LOG("Failed to load content scene %d [%s]\n", index, name);  \
            return false;  \
        } \
        gameState->scene.contentScenesLoaded[index] = true;  \
        gameState->scene.loadingProgress = (float)(index + 1) / AVD_TOTAL_SCENES;  \
        AVD_LOG("Loaded content scene %d [%s]\n", index, name);  \
        if (gameState->scene.allContentScenesLoaded) {  \
            AVD_LOG("All content scenes loaded\n");  \
        }  \
        return true; \
    }


bool fakeLoad() {
    _sleep(100);
    return true;
}

// NOTE: This is a crap way to do this, but I dont wanna spend too much time on it building a proper asset loader/manager
bool avdScenesLoadContentScenesAsyncPoll(AVD_GameState *gameState)
{
    AVD_ASSERT(gameState != NULL);

    AVD_LOAD_CONTENT_SCENE(0, "Main Menu", avdScenesMainMenuInit(gameState));
    // AVD_LOAD_CONTENT_SCENE(1, "Prologue", avdScenesPrologueInit(gameState));
    AVD_LOAD_CONTENT_SCENE(2, "Dummy 2", fakeLoad());
    AVD_LOAD_CONTENT_SCENE(3, "Dummy 4", fakeLoad());
    AVD_LOAD_CONTENT_SCENE(4, "Dummy 5", fakeLoad());
    AVD_LOAD_CONTENT_SCENE(5, "Dummy 6", fakeLoad());

    gameState->scene.allContentScenesLoaded = true;
    return true;
}