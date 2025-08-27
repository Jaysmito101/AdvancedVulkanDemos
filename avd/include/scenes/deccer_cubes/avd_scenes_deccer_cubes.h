
#ifndef AVD_SCENES_DECCER_CUBES_H
#define AVD_SCENES_DECCER_CUBES_H

#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneDeccerCubes {
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
} AVD_SceneDeccerCubes;

bool avdSceneDeccerCubesInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneDeccerCubesRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneDeccerCubesUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneDeccerCubesDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneDeccerCubesLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneDeccerCubesInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneDeccerCubesCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneDeccerCubesRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_DECCER_CUBES_H