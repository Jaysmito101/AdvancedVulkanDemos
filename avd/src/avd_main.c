#include "avd_application.h"


int main() {
    AVD_GameState g_GameState;
    
    if (!avdApplicationInit(&g_GameState)) {
        AVD_LOG("Failed to initialize application\n");
        return -1;
    }

    while(avdApplicationIsRunning(&g_GameState)) {
        avdApplicationUpdate(&g_GameState);
    }

    avdApplicationShutdown(&g_GameState);

    AVD_LOG("Application shutdown successfully\n");
    
    return EXIT_SUCCESS;
}