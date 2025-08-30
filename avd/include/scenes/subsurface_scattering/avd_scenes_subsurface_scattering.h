#ifndef AVD_SCENES_SUBSURFACE_SCATTERING_H
#define AVD_SCENES_SUBSURFACE_SCATTERING_H

#include "common/avd_bloom.h"
#include "model/avd_3d_scene.h"
#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneSubsurfaceScatteringModelInfo {
    uint32_t modelIndex;
    char name[64];
    AVD_Vector3 position;
    AVD_Vector3 rotation;
    AVD_Vector3 scale;
    bool hasPBRTextures;
    uint32_t albedoTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t ormTextureIndex;
    uint32_t thicknessTextureIndex;

    AVD_Vector4 lightPositionA;
    AVD_Vector4 lightPositionB;
} AVD_SceneSubsurfaceScatteringModelInfo;

typedef struct AVD_SceneSubsurfaceScattering {
    AVD_SceneType type;
    AVD_RenderableText title;
    AVD_RenderableText info;

    uint32_t sceneWidth;
    uint32_t sceneHeight;

    AVD_VulkanFramebuffer gBuffer;
    AVD_VulkanFramebuffer aoBuffer;
    AVD_VulkanFramebuffer lightingBuffer;
    AVD_VulkanFramebuffer diffusedIrradianceBuffer;

    AVD_VulkanImage alienThicknessMap;
    AVD_VulkanImage buddhaThicknessMap;
    AVD_VulkanImage standfordDragonThicknessMap;
    AVD_VulkanImage buddhaORMMap;
    AVD_VulkanImage buddhaAlbedoMap;
    AVD_VulkanImage buddhaNormalMap;
    AVD_VulkanImage noiseTexture;

    AVD_VulkanBuffer vertexBuffer;

    bool bloomEnabled;

    AVD_3DScene models;

    VkDescriptorSetLayout set0Layout;
    VkDescriptorSet set0;

    VkPipelineLayout gBufferPipelineLayout;
    VkPipeline gBufferPipeline;

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

    AVD_Matrix4x4 projectionMatrix;
    AVD_Matrix4x4 viewMatrix;
    AVD_Matrix4x4 viewProjectionMatrix;

    uint32_t loadStage;
    int32_t renderMode;

    bool isDragging;
    float lastMouseX;
    float lastMouseY;
    float cameraRadius;
    float cameraPhi;
    float cameraTheta;

    float materialRoughness;
    float materialMetallic;
    float translucencyScale;
    float translucencyDistortion;
    float translucencyPower;
    float translucencyAmbientDiffusion;
    float screenSpaceIrradianceScale;
    float bloomThreshold;
    float bloomIntensity;
    float bloomSoftKnee;
    bool useScreenSpaceIrradiance;

    int32_t currentFocusModelIndex;
    AVD_SceneSubsurfaceScatteringModelInfo modelsInfo[3];

    AVD_Bloom bloom;

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