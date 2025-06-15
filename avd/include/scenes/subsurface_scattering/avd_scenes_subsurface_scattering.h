
#ifndef AVD_SCENES_SUBSURFACE_SCATTERING_H
#define AVD_SCENES_SUBSURFACE_SCATTERING_H

#include "model/avd_3d_scene.h"
#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneSubsurfaceScattering {
    AVD_SceneType type;
    AVD_RenderableText title;
    AVD_RenderableText info;

    AVD_VulkanFramebuffer gBuffer;
    bool bloomEnabled;

    AVD_3DScene models;

    VkDescriptorSetLayout set0Layout;
    VkPipelineLayout compositePipelineLayout;
    VkPipeline compositePipeline;

    uint32_t loadStage;
} AVD_SceneSubsurfaceScattering;

bool avdSceneSubsurfaceScatteringInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneSubsurfaceScatteringRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneSubsurfaceScatteringUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneSubsurfaceScatteringDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneSubsurfaceScatteringLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneSubsurfaceScatteringInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneSubsurfaceScatteringCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneSubsurfaceScatteringRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_SUBSURFACE_SCATTERING_H