#include "ps_application.h"
#include "ps_window.h"

bool psApplicationInit(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    gameState->running = true;

    if(!psWindowInit(gameState)) {
        PS_LOG("Failed to initialize window\n");
        return false;
    }

    // Initialize other application components here

    return true;
}

void psApplicationShutdown(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    psWindowShutdown(gameState);
}

bool psApplicationIsRunning(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    return gameState->running;
}

void psApplicationUpdate(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    psWindowPollEvents(gameState);
    psApplicationUpdateWithoutPolling(gameState);
}



void psApplicationUpdateWithoutPolling(PS_GameState *gameState) {
        
}
