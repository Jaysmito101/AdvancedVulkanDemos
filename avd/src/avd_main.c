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

    AVD_Vector3 c =  avdVec3Zero(); // Example usage of a math function
    AVD_Vector2 length = avdVec2Normalize(c);
    
    return EXIT_SUCCESS;
}