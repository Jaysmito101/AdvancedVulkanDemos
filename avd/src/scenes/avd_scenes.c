#include "scenes/avd_scenes.h"
#include "avd_application.h"

static bool __avdCheckSceneApiValidity(AVD_SceneAPI *api)
{
    AVD_CHECK(api->checkIntegrity != NULL);
    AVD_CHECK(api->init != NULL);
    AVD_CHECK(api->render != NULL);
    AVD_CHECK(api->update != NULL);
    AVD_CHECK(api->destroy != NULL);
    AVD_CHECK(api->load != NULL);
    return true;
}

static bool __avdCheckAllSceneApis(AVD_SceneManager *sceneManager)
{
    AVD_ASSERT(sceneManager != NULL);
    for (int i = 0; i < AVD_SCENE_TYPE_COUNT; ++i)
        AVD_CHECK(__avdCheckSceneApiValidity(&sceneManager->api[i]));
    return true;
}

static bool __avdRegisterSceneApis(AVD_SceneManager *sceneManager)
{
    AVD_ASSERT(sceneManager != NULL);
    avdSceneMainMenuRegisterApi(&sceneManager->api[AVD_SCENE_TYPE_MAIN_MENU]);
    return true;
}

bool avdSceneManagerInit(AVD_SceneManager *sceneManager, AVD_AppState *appState)
{
    AVD_CHECK(__avdRegisterSceneApis(sceneManager));
    AVD_CHECK(__avdCheckAllSceneApis(sceneManager));

    sceneManager->currentSceneType = AVD_SCENE_TYPE_MAIN_MENU;
    sceneManager->isSceneInitialized = false;
    AVD_CHECK(avdSceneManagerSwitchToScene(sceneManager, AVD_SCENE_TYPE_MAIN_MENU, appState));

    return true;
}

void avdSceneManagerDestroy(AVD_SceneManager *sceneManager, AVD_AppState *appState)
{
    AVD_ASSERT(sceneManager != NULL);
    AVD_ASSERT(appState != NULL);

    if (sceneManager->isSceneInitialized)
    {
        sceneManager->api[sceneManager->currentSceneType].destroy(appState, &sceneManager->scene);
        sceneManager->isSceneInitialized = false;
        sceneManager->isSceneLoaded = false;
    }
}

bool avdSceneManagerUpdate(AVD_SceneManager *sceneManager, AVD_AppState *appState)
{
    AVD_ASSERT(sceneManager != NULL);
    AVD_ASSERT(appState != NULL);

    if (sceneManager->isSceneInitialized)
    {
        if (sceneManager->isSceneLoaded)
        {
            AVD_CHECK(sceneManager->api[sceneManager->currentSceneType].update(appState, &sceneManager->scene));
        }
        else
        {
            sceneManager->isSceneLoaded = sceneManager->api[sceneManager->currentSceneType].load(appState, &sceneManager->scene, &sceneManager->sceneLoadingStatusMessage, &sceneManager->sceneLoadingProgress);
        }
    }

    return true;
}

bool avdSceneManagerRender(AVD_SceneManager *sceneManager, AVD_AppState *appState)
{
    AVD_ASSERT(sceneManager != NULL);
    AVD_ASSERT(appState != NULL);

    if (sceneManager->isSceneInitialized && sceneManager->isSceneLoaded)
    {
        AVD_CHECK(sceneManager->api[sceneManager->currentSceneType].render(appState, &sceneManager->scene));
    }

    return true;
}

bool avdSceneManagerSwitchToScene(AVD_SceneManager *sceneManager, AVD_SceneType type, AVD_AppState *appState)
{
    if (!sceneManager->api[type].checkIntegrity(appState, &sceneManager->sceneIntegrityStatusMessage))
    {
        sceneManager->sceneIntegrityCheckPassed = false;
        AVD_LOG("Scene integrity check failed: %s [Switch Cancelled]\n", sceneManager->sceneIntegrityStatusMessage);
        return true;
    }

    sceneManager->sceneIntegrityCheckPassed = true;

    if (sceneManager->isSceneInitialized)
        sceneManager->api[sceneManager->currentSceneType].destroy(appState, &sceneManager->scene);
    memset(&sceneManager->scene, 0, sizeof(AVD_Scene));

    // Very important to set the type before calling init
    sceneManager->currentSceneType = type;
    sceneManager->scene.type = type;

    AVD_CHECK(sceneManager->api[type].init(appState, &sceneManager->scene));

    sceneManager->isSceneInitialized = true;
    sceneManager->isSceneLoaded = false;
    sceneManager->sceneLoadingProgress = 0.0f;
    sceneManager->sceneLoadingStatusMessage = NULL;
    return true;
}
