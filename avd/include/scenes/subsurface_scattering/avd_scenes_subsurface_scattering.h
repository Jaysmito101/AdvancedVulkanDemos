#ifndef AVD_SCENES_SUBSURFACE_SCATTERING_H
#define AVD_SCENES_SUBSURFACE_SCATTERING_H

#include "model/avd_3d_scene.h"
#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneSubsurfaceScattering {
    AVD_SceneType type;
    AVD_RenderableText title;
    AVD_RenderableText info;

    uint32_t sceneWidth;
    uint32_t sceneHeight;

    AVD_VulkanFramebuffer depthBuffer;
    AVD_VulkanFramebuffer aoBuffer;
    AVD_VulkanFramebuffer lightingBuffer;
    AVD_VulkanFramebuffer diffusedIrradianceBuffer;

    AVD_VulkanImage alienThicknessMap;
    AVD_VulkanImage buddhaThicknessMap;
    AVD_VulkanImage standfordDragonThicknessMap;

    AVD_VulkanBuffer vertexBuffer;

    bool bloomEnabled;

    AVD_3DScene models;

    VkDescriptorSetLayout set0Layout;
    VkDescriptorSet set0;

    VkPipelineLayout depthPipelineLayout;
    VkPipeline depthPipeline;

    VkPipelineLayout aoPipelineLayout;
    VkPipeline aoPipeline;

    VkPipelineLayout lightingPipelineLayout;
    VkPipeline lightingPipeline;

    VkPipelineLayout irradianceDiffusionPipelineLayout;
    VkPipeline irradianceDiffusionPipeline;

    VkPipelineLayout compositePipelineLayout;
    VkPipeline compositePipeline;

    AVD_Vector3 cameraPosition;
    AVD_Vector3 cameraTarget;

    AVD_Vector3 alienPosition;
    AVD_Vector3 buddhaPosition;
    AVD_Vector3 standfordDragonPosition;

    AVD_Matrix4x4 projectionMatrix;
    AVD_Matrix4x4 viewMatrix;

    uint32_t loadStage;

    bool isDragging;
    float lastMouseX;
    float lastMouseY;
    float cameraRadius;
    float cameraPhi;
    float cameraTheta;
    int32_t currentFocusModelIndex; // 0: Dragon, 1: Alien, 2: Buddha
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