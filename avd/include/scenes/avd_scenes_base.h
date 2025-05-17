#ifndef AVD_SCENES_BASE_H
#define AVD_SCENES_BASE_H

#include "core/avd_core.h"
#include "vulkan/avd_vulkan.h"
#include "shader/avd_shader.h"
#include "font/avd_font_renderer.h"


typedef enum AVD_SceneType
{ 
    AVD_SCENE_TYPE_MAIN_MENU,
    AVD_SCENE_TYPE_COUNT
} AVD_SceneType;

struct AVD_AppState;
union AVD_Scene;

// The scene api
typedef bool (*AVD_SceneCheckIntegrityFn)(struct AVD_AppState *appState, const char** statusMessage);
typedef bool (*AVD_SceneInitFn)(struct AVD_AppState *appState, union AVD_Scene *scene);
typedef bool (*AVD_SceneRenderFn)(struct AVD_AppState *appState, union AVD_Scene *scene);
typedef bool (*AVD_SceneUpdateFn)(struct AVD_AppState *appState, union AVD_Scene *scene);
typedef void (*AVD_SceneDestroyFn)(struct AVD_AppState *appState, union AVD_Scene *scene);
typedef bool (*AVD_SceneLoaderFn)(struct AVD_AppState *appState, union AVD_Scene *scene, const char** statusMessage, float* progress);
typedef void (*AVD_SceneInputEventFn)(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent* event);

typedef struct AVD_SceneAPI
{
    AVD_SceneCheckIntegrityFn checkIntegrity;
    AVD_SceneInitFn init;
    AVD_SceneRenderFn render;
    AVD_SceneUpdateFn update;
    AVD_SceneDestroyFn destroy;
    AVD_SceneLoaderFn load;
    AVD_SceneInputEventFn inputEvent;
} AVD_SceneAPI;

#endif // AVD_SCENES_BASE_H