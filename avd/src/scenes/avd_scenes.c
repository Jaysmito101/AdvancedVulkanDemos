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
    AVD_CHECK(api->inputEvent != NULL);

    AVD_CHECK(api->id != NULL);
    if (api->displayName == NULL) {
        AVD_LOG("Scene API '%s' does not have a display name set, using id as display name.\n", api->id);
        api->displayName = api->id;
    }
    return true;
}

static bool __avdCheckAllSceneApis(AVD_SceneManager *sceneManager)
{
    AVD_ASSERT(sceneManager != NULL);
    for (int i = 0; i < AVD_SCENE_TYPE_COUNT; ++i)
        AVD_CHECK_MSG(__avdCheckSceneApiValidity(&sceneManager->api[i]), "Failed to validate scene API for type %s\n",
                      avdSceneTypeToString((AVD_SceneType)i));
    return true;
}

static bool __avdRegisterSceneApis(AVD_SceneManager *sceneManager)
{
    AVD_ASSERT(sceneManager != NULL);
    avdSceneMainMenuRegisterApi(&sceneManager->api[AVD_SCENE_TYPE_MAIN_MENU]);
    avdSceneBloomRegisterApi(&sceneManager->api[AVD_SCENE_TYPE_BLOOM]);
    avdScene2DRadianceCascadesRegisterApi(&sceneManager->api[AVD_SCENE_TYPE_2D_RADIANCE_CASCADES]);
    avdSceneDeccerCubesRegisterApi(&sceneManager->api[AVD_SCENE_TYPE_DECCER_CUBES]);
    return true;
}

bool avdSceneManagerInit(AVD_SceneManager *sceneManager, AVD_AppState *appState)
{
    AVD_CHECK(__avdRegisterSceneApis(sceneManager));
    AVD_CHECK(__avdCheckAllSceneApis(sceneManager));

    sceneManager->currentSceneType   = AVD_SCENE_TYPE_MAIN_MENU;
    sceneManager->isSceneInitialized = false;
    AVD_CHECK(avdSceneManagerSwitchToScene(sceneManager, AVD_SCENE_TYPE_MAIN_MENU, appState));

    return true;
}

void avdSceneManagerDestroy(AVD_SceneManager *sceneManager, AVD_AppState *appState)
{
    AVD_ASSERT(sceneManager != NULL);
    AVD_ASSERT(appState != NULL);

    if (sceneManager->isSceneInitialized) {
        sceneManager->api[sceneManager->currentSceneType].destroy(appState, &sceneManager->scene);
        sceneManager->isSceneInitialized = false;
        sceneManager->isSceneLoaded      = false;
    }
}

bool avdSceneManagerUpdate(AVD_SceneManager *sceneManager, AVD_AppState *appState)
{
    AVD_ASSERT(sceneManager != NULL);
    AVD_ASSERT(appState != NULL);

    if (sceneManager->isSceneInitialized) {
        if (sceneManager->isSceneLoaded) {
            AVD_CHECK(sceneManager->api[sceneManager->currentSceneType].update(appState, &sceneManager->scene));
        } else {
            sceneManager->isSceneLoaded = sceneManager->api[sceneManager->currentSceneType].load(appState, &sceneManager->scene, &sceneManager->sceneLoadingStatusMessage, &sceneManager->sceneLoadingProgress);
        }
    }

    return true;
}

bool avdSceneManagerRender(AVD_SceneManager *sceneManager, AVD_AppState *appState)
{
    AVD_ASSERT(sceneManager != NULL);
    AVD_ASSERT(appState != NULL);

    if (sceneManager->isSceneInitialized && sceneManager->isSceneLoaded) {
        AVD_CHECK(sceneManager->api[sceneManager->currentSceneType].render(appState, &sceneManager->scene));
    }

    return true;
}

void avdSceneManagerPushInputEvent(AVD_SceneManager *sceneManager, struct AVD_AppState *appState, AVD_InputEvent *event)
{
    AVD_ASSERT(sceneManager != NULL);
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(event != NULL);

    if (sceneManager->isSceneInitialized) {
        sceneManager->api[sceneManager->currentSceneType].inputEvent(appState, &sceneManager->scene, event);
    }
}

bool avdSceneManagerSwitchToScene(AVD_SceneManager *sceneManager, AVD_SceneType type, AVD_AppState *appState)
{
    if (!sceneManager->api[type].checkIntegrity(appState, &sceneManager->sceneIntegrityStatusMessage)) {
        sceneManager->sceneIntegrityCheckPassed = false;
        AVD_LOG("Scene integrity check failed: %s [Switch Cancelled]\n", sceneManager->sceneIntegrityStatusMessage);
        avdMessageBox(
            "Scene Integrity Check Failed",
            sceneManager->sceneIntegrityStatusMessage ? sceneManager->sceneIntegrityStatusMessage : "No specific message provided."
        );
        return true;
    }

    sceneManager->sceneIntegrityCheckPassed = true;

    if (sceneManager->isSceneInitialized) {
        avdVulkanWaitIdle(&appState->vulkan);
        sceneManager->api[sceneManager->currentSceneType].destroy(appState, &sceneManager->scene);
    }
    memset(&sceneManager->scene, 0, sizeof(AVD_Scene));

    // Very important to set the type before calling init
    sceneManager->currentSceneType = type;
    sceneManager->scene.type       = type;

    AVD_CHECK(sceneManager->api[type].init(appState, &sceneManager->scene));

    sceneManager->isSceneInitialized        = true;
    sceneManager->isSceneLoaded             = false;
    sceneManager->sceneLoadingProgress      = 0.0f;
    sceneManager->sceneLoadingStatusMessage = NULL;
    return true;
}

const char *avdSceneTypeToString(AVD_SceneType type)
{
    switch (type) {
        case AVD_SCENE_TYPE_MAIN_MENU:
            return "Main_Menu";
        case AVD_SCENE_TYPE_BLOOM:
            return "Bloom";
        case AVD_SCENE_TYPE_2D_RADIANCE_CASCADES:
            return "2D_Radiance_Cascades";
        default:
            return "Unknown_Scene_Type";
    }
}