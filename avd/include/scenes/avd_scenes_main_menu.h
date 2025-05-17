#ifndef AVD_SCENES_MAIN_MENU_H
#define AVD_SCENES_MAIN_MENU_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneMainMenuCard {
    AVD_VulkanImage thumbnailImage;
    AVD_RenderableText title;
    
    VkDescriptorSet descriptorSet;
} AVD_SceneMainMenuCard;

typedef struct AVD_SceneMainMenu
{
    AVD_SceneType type;

    AVD_SceneMainMenuCard cards[64];
    uint32_t cardCount;
    
    AVD_RenderableText title;
    AVD_RenderableText creditsText;  
    AVD_RenderableText githubLinkText;

    int32_t loadingCount;
} AVD_SceneMainMenu;

bool avdSceneMainMenuInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneMainMenuRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneMainMenuUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneMainMenuDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneMainMenuLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char** statusMessage, float* progress);
void avdSceneMainMenuInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent* event);

bool avdSceneMainMenuCheckIntegrity(struct AVD_AppState *appState, const char** statusMessage);
bool avdSceneMainMenuRegisterApi(AVD_SceneAPI *api);



#endif // AVD_SCENES_MAIN_MENU_H