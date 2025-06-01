#include "avd_application.h"

#include "math/avd_math_tests.h"

int main()
{
    AVD_AppState g_appState = {0};

    AVD_CHECK(avdMathTestsRun());

    if (!avdApplicationInit(&g_appState)) {
        AVD_LOG("Failed to initialize application\n");
        return -1;
    }

    while (avdApplicationIsRunning(&g_appState)) {
        avdApplicationUpdate(&g_appState);
    }

    avdApplicationShutdown(&g_appState);

    AVD_LOG("Application shutdown successfully\n");

    
    return EXIT_SUCCESS;
}