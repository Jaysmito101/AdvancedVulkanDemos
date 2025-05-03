#ifndef PS_GAME_STATE_H
#define PS_GAME_STATE_H

struct GLFWwindow;

typedef struct PS_Window {
    struct GLFWwindow* window;


} PS_Window;

typedef struct PS_GameState {
    PS_Window window;

    bool running;
} PS_GameState;


#endif // PS_GAME_STATE_H