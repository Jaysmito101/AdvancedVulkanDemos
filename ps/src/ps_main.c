#include "ps_application.h"


int main() {
    PS_GameState g_GameState;
    
    if (!psApplicationInit(&g_GameState)) {
        PS_LOG("Failed to initialize application\n");
        return -1;
    }

    while(psApplicationIsRunning(&g_GameState)) {
        psApplicationUpdate(&g_GameState);
    }

    psApplicationShutdown(&g_GameState);

    PS_LOG("Application shutdown successfully\n");
    
    return EXIT_SUCCESS;
}