#ifndef PS_GAME_STATE_H
#define PS_GAME_STATE_H

struct GLFWwindow;

typedef struct PS_Window {
    struct GLFWwindow* window;
    int32_t width;
    int32_t height;

    int32_t framebufferWidth;
    int32_t framebufferHeight;
} PS_Window;

typedef struct PS_GameState {
    PS_Window window;

    bool running;
} PS_GameState;


#endif // PS_GAME_STATE_H