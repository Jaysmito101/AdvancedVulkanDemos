#ifndef AVD_SCENES_REALISTIC_HEAD_H
#define AVD_SCENES_REALISTIC_HEAD_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneRealisticHead {
    AVD_SceneType type;

    AVD_Float sceneWidth;
    AVD_Float sceneHeight;
    AVD_Matrix4x4 viewModelMatrix;
    AVD_Matrix4x4 projectionMatrix;

    AVD_VulkanBuffer indexBuffer;
    AVD_VulkanBuffer vertexBuffer;

    AVD_RenderableText title;
    AVD_RenderableText info;

    AVD_3DScene models;

    VkDescriptorSetLayout set0Layout;
    VkDescriptorSet set0;

    uint32_t loadStage;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

} AVD_SceneRealisticHead;

bool avdSceneRealisticHeadInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneRealisticHeadRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneRealisticHeadUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneRealisticHeadDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneRealisticHeadLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneRealisticHeadInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneRealisticHeadCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneRealisticHeadRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_REALISTIC_HEAD_H