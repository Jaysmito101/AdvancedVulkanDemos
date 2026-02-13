#ifndef AVD_SCENES_BASE_H
#define AVD_SCENES_BASE_H

#include "core/avd_core.h"
#include "font/avd_font_renderer.h"
#include "geom/avd_geom.h"
#include "model/avd_3d_scene.h"
#include "shader/avd_shader.h"
#include "vulkan/avd_vulkan.h"

typedef enum AVD_SceneType {
    AVD_SCENE_TYPE_MAIN_MENU,
    AVD_SCENE_TYPE_REALISTIC_HEAD,
    AVD_SCENE_TYPE_SUBSURFACE_SCATTERING,
    AVD_SCENE_TYPE_HLS_PLAYER,
    AVD_SCENE_TYPE_BLOOM,
    AVD_SCENE_TYPE_EYEBALLS,
    AVD_SCENE_TYPE_2D_RADIANCE_CASCADES,
    AVD_SCENE_TYPE_DECCER_CUBES,
    AVD_SCENE_TYPE_COUNT
} AVD_SceneType;

struct AVD_AppState;
union AVD_Scene;

// The scene api
typedef bool (*AVD_SceneCheckIntegrityFn)(struct AVD_AppState *appState, const char **statusMessage);
typedef bool (*AVD_SceneInitFn)(struct AVD_AppState *appState, union AVD_Scene *scene);
typedef bool (*AVD_SceneRenderFn)(struct AVD_AppState *appState, union AVD_Scene *scene);
typedef bool (*AVD_SceneUpdateFn)(struct AVD_AppState *appState, union AVD_Scene *scene);
typedef void (*AVD_SceneDestroyFn)(struct AVD_AppState *appState, union AVD_Scene *scene);
typedef bool (*AVD_SceneLoaderFn)(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
typedef void (*AVD_SceneInputEventFn)(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

typedef struct AVD_SceneAPI {
    const char *displayName; // Name of the scene
    const char *id;          // Unique identifier for the scene type

    AVD_SceneCheckIntegrityFn checkIntegrity;
    AVD_SceneInitFn init;
    AVD_SceneRenderFn render;
    AVD_SceneUpdateFn update;
    AVD_SceneDestroyFn destroy;
    AVD_SceneLoaderFn load;
    AVD_SceneInputEventFn inputEvent;
} AVD_SceneAPI;

const char *avdSceneTypeToString(AVD_SceneType type);

#endif // AVD_SCENES_BASE_H