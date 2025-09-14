#include "avd_application.h"

#include "math/avd_math_tests.h"

int main()
{
    AVD_AppState *appState = (AVD_AppState *)malloc(sizeof(AVD_AppState));
    AVD_CHECK_MSG(appState != NULL, "Failed to allocate memory for application state\n");
    memset(appState, 0, sizeof(AVD_AppState));

#ifdef AVD_DEBUG
    // Run tests only in debug mode for now
    AVD_CHECK(avdMathTestsRun());
    AVD_CHECK(avdListTestsRun());
    AVD_CHECK(avdHashTableTestsRun());
#endif

    if (!avdApplicationInit(appState)) {
        AVD_LOG("Failed to initialize application\n");
        return -1;
    }

    while (avdApplicationIsRunning(appState)) {
        avdApplicationUpdate(appState);
    }

    avdApplicationShutdown(appState);

    AVD_LOG("Application shutdown successfully\n");

    return EXIT_SUCCESS;
}