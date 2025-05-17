#include "avd_application.h"

int main()
{
    AVD_AppState g_appState = {0};

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