#include "avd_application.h"

#include "math/avd_math_tests.h"

int main()
{
    AVD_AppState* appState = (AVD_AppState*)malloc(sizeof(AVD_AppState));
    if (appState == NULL) {
        AVD_LOG("Failed to allocate memory for application state\n");
        return -1;
    }

#ifdef AVD_DEBUG
    // Run tests only in debug mode for now
    AVD_CHECK(avdMathTestsRun());
    AVD_CHECK(avdListTestsRun());
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