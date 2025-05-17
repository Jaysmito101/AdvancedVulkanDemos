#ifndef AVD_WINDOW_CB_H
#define AVD_WINDOW_CB_H

#include "avd_application.h"
#include "core/avd_window.h"

void __avdGLFWErrorCallback(int error, const char *description);
void __avdGLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void __avdGLFWCharCallback(GLFWwindow *window, unsigned int codepoint);
void __avdGLFWDropCallback(GLFWwindow *window, int count, const char **paths);
void __avdGLFWScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void __avdGLFWCursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void __avdGLFWWindowPosCallback(GLFWwindow *window, int xpos, int ypos);
void __avdGLFWWindowSizeCallback(GLFWwindow *window, int width, int height);
void __avdGLFWCursorEnterCallback(GLFWwindow *window, int entered);
void __avdGLFWWindowCloseCallback(GLFWwindow *window);
void __avdGLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void __avdGLFWFramebufferSizeCallback(GLFWwindow *window, int width, int height);
void __avdGLFWWindowMaximizeCallback(GLFWwindow *window, int maximized);
void __avdGLFWWindowFocusCallback(GLFWwindow *window, int focused);
void __avdSetupWindowEvents(AVD_Window *window);

#endif // AVD_WINDOW_CB_H