#ifndef AVD_UI_H
#define AVD_UI_H

#include "core/avd_core.h"
#include "font/avd_font_renderer.h"
#include "shader/avd_shader.h"
#include "vulkan/avd_vulkan.h"

struct AVD_AppState;

typedef enum AVD_SimpleUiElementType {
    AVD_UI_ELEMENT_TYPE_NONE = 0,
    AVD_UI_ELEMENT_TYPE_RECT = 1,
} AVD_SimpleUiElementType;

typedef struct AVD_SimpleUi {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;

    float width;
    float height;
    float offsetX;
    float offsetY;

    float frameWidth;
    float frameHeight;
} AVD_SimpleUi;

bool avdSimpleUiInit(AVD_SimpleUi *ui, struct AVD_AppState *appState);
void avdSimpleUiDestroy(AVD_SimpleUi *ui, struct AVD_AppState *appState);

void avdSimpleUiBegin(VkCommandBuffer commandBuffer, AVD_SimpleUi *ui, struct AVD_AppState *appState, float width, float height, float offsetX, float offsetY, uint32_t frameWidth, uint32_t frameHeight);
void avdSimpleUiEnd(VkCommandBuffer commandBuffer, AVD_SimpleUi *ui, struct AVD_AppState *appState);
void avdSimpleUiDrawRect(
    VkCommandBuffer commandBuffer,
    AVD_SimpleUi *ui,
    struct AVD_AppState *appState,
    float x,
    float y,
    float width,
    float height,
    float r,
    float g,
    float b,
    float a,
    VkDescriptorSet descriptorSet,
    uint32_t imageWidth,
    uint32_t imageHeight);

#endif // AVD_UI_H