#ifndef AVD_WINDOW_CB_H
#define AVD_WINDOW_CB_H

#include "avd_application.h"
#include "core/avd_window.h"

void PRIV_avdGLFWErrorCallback(int error, const char *description);
void PRIV_avdGLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void PRIV_avdGLFWCharCallback(GLFWwindow *window, unsigned int codepoint);
void PRIV_avdGLFWDropCallback(GLFWwindow *window, int count, const char **paths);
void PRIV_avdGLFWScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void PRIV_avdGLFWCursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void PRIV_avdGLFWWindowPosCallback(GLFWwindow *window, int xpos, int ypos);
void PRIV_avdGLFWWindowSizeCallback(GLFWwindow *window, int width, int height);
void PRIV_avdGLFWCursorEnterCallback(GLFWwindow *window, int entered);
void PRIV_avdGLFWWindowCloseCallback(GLFWwindow *window);
void PRIV_avdGLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void PRIV_avdGLFWFramebufferSizeCallback(GLFWwindow *window, int width, int height);
void PRIV_avdGLFWWindowMaximizeCallback(GLFWwindow *window, int maximized);
void PRIV_avdGLFWWindowFocusCallback(GLFWwindow *window, int focused);
void PRIV_avdSetupWindowEvents(AVD_Window *window);

#endif // AVD_WINDOW_CB_H