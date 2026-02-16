#ifndef AVD_SCENES_UI_SYSTEM_DEMO_H
#define AVD_SCENES_UI_SYSTEM_DEMO_H

#include "common/avd_common.h"
#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneUiSystemDemo {
    AVD_SceneType type;
    AVD_Gui gui;

    AVD_GuiWindowType currentWindowType;
    AVD_GuiLayoutAlign currentHAlign;
    AVD_GuiLayoutAlign currentVAlign;

    float demoSliderValue;
    bool demoCheckboxValue;
    int demoCounter;
} AVD_SceneUiSystemDemo;

bool avdSceneUiSystemDemoInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneUiSystemDemoRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneUiSystemDemoUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneUiSystemDemoDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneUiSystemDemoLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneUiSystemDemoInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneUiSystemDemoCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneUiSystemDemoRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_UI_SYSTEM_DEMO_H
