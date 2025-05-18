#ifndef AVD_SCENES_BLOOM_H
#define AVD_SCENES_BLOOM_H

#include "bloom/avd_bloom.h"
#include "scenes/avd_scenes_base.h"

typedef struct AVD_SceneBloom {
    AVD_SceneType type;

    AVD_RenderableText title;
    AVD_RenderableText uiInfoText;

    VkDescriptorSetLayout descriptorSetLayout;
    AVD_BloomPrefilterType prefilterType;
    float bloomThreshold;
    float bloomSoftKnee;
    float bloomAmount;
    bool lowQuality;
    bool applyGamma;
    AVD_BloomTonemappingType tonemappingType;

    bool isBloomEnabled;
} AVD_SceneBloom;

bool avdSceneBloomInit(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneBloomRender(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneBloomUpdate(struct AVD_AppState *appState, union AVD_Scene *scene);
void avdSceneBloomDestroy(struct AVD_AppState *appState, union AVD_Scene *scene);
bool avdSceneBloomLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress);
void avdSceneBloomInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event);

bool avdSceneBloomCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage);
bool avdSceneBloomRegisterApi(AVD_SceneAPI *api);

#endif // AVD_SCENES_BLOOM_H