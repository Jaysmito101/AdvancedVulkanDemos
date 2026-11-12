#include "avd_application.h"
#include "avd_cli.h"

#include "math/avd_math_tests.h"

int main(int argc, char **argv)
{
    AVD_SceneType initialScene = AVD_SCENE_TYPE_MAIN_MENU;

    AVD_CliStatus cliStatus = avdCliParse(argc, argv, &initialScene);
    if (cliStatus != AVD_CLI_STATUS_OK) {
        return cliStatus == AVD_CLI_STATUS_HELP ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    AVD_LOG_INIT();

    AVD_AppState *appState = (AVD_AppState *)malloc(sizeof(AVD_AppState));
    AVD_CHECK_MSG(appState != NULL, "Failed to allocate memory for application state\n");
    memset(appState, 0, sizeof(AVD_AppState));

#ifdef AVD_DEBUG
    // Run tests only in debug mode for now
    AVD_CHECK(avdMathTestsRun());
    AVD_CHECK(avdListTestsRun());
    AVD_CHECK(avdHashTableTestsRun());
    // AVD_CHECK(avdCurlUtilsTestsRun());
#endif

    if (!avdApplicationInit(appState)) {
        AVD_LOG_ERROR("Failed to initialize application\n");
        return -1;
    }

    if (initialScene != AVD_SCENE_TYPE_MAIN_MENU) {
        avdSceneManagerSwitchToScene(&appState->sceneManager, initialScene, appState);
    }

    while (avdApplicationIsRunning(appState)) {
        avdApplicationUpdate(appState);
    }

    avdApplicationShutdown(appState);

    AVD_LOG_VERBOSE("Application shutdown successfully\n");

    AVD_LOG_SHUTDOWN();

    return EXIT_SUCCESS;
}