#ifndef AVD_UI_H
#define AVD_UI_H

#include "core/avd_core.h"
#include "vulkan/avd_vulkan.h"
#include "shader/avd_shader.h"
#include "font/avd_font_renderer.h"

struct AVD_AppState;

typedef enum AVD_UiElementType
{
    AVD_UI_ELEMENT_TYPE_NONE = 0,
    AVD_UI_ELEMENT_TYPE_RECT = 1,
} AVD_UiElementType;

typedef struct AVD_Ui {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;

    float width;
    float height;
    float offsetX;
    float offsetY;

    float frameWidth;
    float frameHeight;
} AVD_Ui;


bool avdUiInit(AVD_Ui *ui, struct AVD_AppState *appState);
void avdUiDestroy(AVD_Ui *ui, struct AVD_AppState *appState);


void avdUiBegin(VkCommandBuffer commandBuffer, AVD_Ui *ui, struct AVD_AppState *appState, float width, float height, float offsetX, float offsetY, uint32_t frameWidth, uint32_t frameHeight);
void avdUiEnd(VkCommandBuffer commandBuffer, AVD_Ui *ui, struct AVD_AppState *appState);
void avdUiDrawRect(VkCommandBuffer commandBuffer, AVD_Ui *ui, struct AVD_AppState *appState, float x, float y, float width, float height, float r, float g, float b, float a);


#endif // AVD_UI_H