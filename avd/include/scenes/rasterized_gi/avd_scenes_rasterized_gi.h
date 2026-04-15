
#ifndef AVD_SCENES_RASTERIZED_GI_H
#define AVD_SCENES_RASTERIZED_GI_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneRasterizedGI {
    AVD_SceneType type;
    AVD_3DScene scene;

    AVD_Matrix4x4 projectionMatrix;
    AVD_Matrix4x4 viewMatrix;

    AVD_RenderableText title;
    AVD_RenderableText info;
    AVD_UInt32 loadStage;

    VkDescriptorSetLayout set0Layout;
    VkDescriptorSet set0;

    AVD_VulkanBuffer vertexBuffer;
    AVD_VulkanBuffer indexBuffer;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    AVD_UInt32 imagesHashes[16];
    AVD_VulkanImage images[16];
    AVD_UInt32 imagesCount;
} AVD_SceneRasterizedGI;

bool avdSceneRasterizedGIInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneRasterizedGIRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneRasterizedGIUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneRasterizedGIDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneRasterizedGILoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneRasterizedGIInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneRasterizedGICheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneRasterizedGIRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_RASTERIZED_GI_H