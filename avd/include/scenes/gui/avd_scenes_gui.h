#ifndef AVD_SCENES_GUI_H
#define AVD_SCENES_GUI_H

#include "common/avd_drawlist.h"
#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneImmediateGui {
    AVD_SceneType type;

    AVD_RenderableText title;
    AVD_RenderableText uiInfoText;

    AVD_DrawList drawList;
    AVD_DrawListRenderer drawListRenderer;

} AVD_SceneImmediateGui;

bool avdSceneImmediateGuiInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneImmediateGuiRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneImmediateGuiUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneImmediateGuiDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneImmediateGuiLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneImmediateGuiInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneImmediateGuiCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneImmediateGuiRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_GUI_H