#ifndef AVD_GUI_H
#define AVD_GUI_H

#include "avd_drawlist.h"
#include "core/avd_types.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_framebuffer.h"
#include "vulkan/avd_vulkan_image.h"

typedef enum {
    AVD_GUI_WINDOW_FLAGS_NONE = 0,
    AVD_GUI_WINDOW_FLAGS_NO_TITLE_BAR = 1 << 0,
    AVD_GUI_WINDOW_FLAGS_NO_RESIZE = 1 << 1,
    AVD_GUI_WINDOW_FLAGS_NO_MOVE = 1 << 2,
    AVD_GUI_WINDOW_FLAGS_NO_SCROLLBAR = 1 << 3,
    AVD_GUI_WINDOW_FLAGS_NO_COLLAPSE = 1 << 4,
} AVD_GuiWindowFlags;

typedef enum {
    AVD_GUI_CHILD_FLAGS_NONE = 0,
    AVD_GUI_CHILD_FLAGS_NO_SCROLLBAR = 1 << 0,
} AVD_GuiChildFlags;

typedef enum {
    AVD_GUI_STYLE_COLOR_TEXT = 0,
    AVD_GUI_STYLE_COLOR_WINDOW_BACKGROUND,
    AVD_GUI_STYLE_COLOR_BUTTON,
    AVD_GUI_STYLE_COLOR_BUTTON_HOVERED,
    AVD_GUI_STYLE_COLOR_BUTTON_ACTIVE,
    // ... add more style colors as needed
} AVD_GuiStyleColor;

typedef enum {
    AVD_GUI_STYLE_VAR_NONE = 0,
    AVD_GUI_STYLE_VAR_WINDOW_PADDING,
    AVD_GUI_STYLE_VAR_ITEM_SPACING,
    // ... add more style vars as needed
} AVD_GuiStyleVar;

typedef enum {
    AVD_GUI_DIR_UP = 0,
    AVD_GUI_DIR_DOWN,
    AVD_GUI_DIR_LEFT,
    AVD_GUI_DIR_RIGHT,
} AVD_GuiDir;


typedef struct AVD_Gui {
    int dummy;
} AVD_Gui;



AVD_Bool avdGuiCreate(AVD_Gui* gui, AVD_Vulkan* vulkan, AVD_FontRenderer* fontRenderer, AVD_VulkanFramebuffer* framebuffer);
void avdGuiDestroy(AVD_Gui* gui);


AVD_Bool avdGuiBeginFrame(AVD_Gui* gui);
void avdGuiEndFrame(AVD_Gui* gui);
AVD_Bool avdGuiRender(AVD_Gui* gui, VkCommandBuffer commandBuffer);

AVD_Bool avdGuiBeginWindow(AVD_Gui* gui, const char* title, AVD_Bool* open, AVD_GuiWindowFlags flags);
void avdGuiEndWindow(AVD_Gui* gui);


AVD_Bool avdGuiBeginChild(AVD_Gui* gui, const char* id, AVD_Vector2 size, AVD_GuiChildFlags flags);
void avdGuiEndChild(AVD_Gui* gui);

AVD_Bool avdGuiIsWindowCollapsed(AVD_Gui* gui);
AVD_Bool avdGuiIsWindowFocused(AVD_Gui* gui);
AVD_Bool avdGuiIsWindowHovered(AVD_Gui* gui);
AVD_DrawList* avdGuiGetWindowDrawList(AVD_Gui* gui);
AVD_Vector2 avdGuiGetWindowSize(AVD_Gui* gui);
AVD_Vector2 avdGuiGetWindowPosition(AVD_Gui* gui);
AVD_Vector2 avdGuiGetWindowScroll(AVD_Gui* gui);
AVD_Vector2 avdGuiWindowGetMaximumScroll(AVD_Gui* gui);

void avdGuiSetWindowSize(AVD_Gui* gui, AVD_Vector2 size);
void avdGuiSetWindowPosition(AVD_Gui* gui, AVD_Vector2 position);
void avdGuiSetWindowCollapsed(AVD_Gui* gui, AVD_Bool collapsed);
void avdGuiSetWindowFocus(AVD_Gui* gui);
void avdGuiSetWindowScroll(AVD_Gui* gui, AVD_Vector2 scroll);

AVD_Bool avdGuiPushFont(AVD_Gui* gui, const char* fontName);
void avdGuiPopFont(AVD_Gui* gui);

AVD_Bool avdGuiPushStyleColor(AVD_Gui* gui, AVD_GuiStyleColor styleColor, AVD_Vector3 color);
void avdGuiPopStyleColor(AVD_Gui* gui);

AVD_Bool avdGuiPushStyleVar(AVD_Gui* gui, AVD_GuiStyleVar styleVar, float value);
void avdGuiPopStyleVar(AVD_Gui* gui);

AVD_Bool avdGuiPushId(AVD_Gui* gui, const char* id);
AVD_Bool avdGuiPushIdI32(AVD_Gui* gui, AVD_Int32 id);
void avdGuiPopId(AVD_Gui* gui);

AVD_Vector2 avdGuiGetCursorPosition(AVD_Gui* gui);

AVD_Bool avdGuiShowDemoWindow(AVD_Gui* gui, AVD_Bool* open);
AVD_Bool avdGuiShowMetricsWindow(AVD_Gui* gui, AVD_Bool* open);
AVD_Bool avdGuiShowStyleEditor(AVD_Gui* gui, AVD_Bool* open);
AVD_Bool avdGuiShowAboutWindow(AVD_Gui* gui, AVD_Bool* open);

void avdGuiStyleColorsDark(AVD_Gui* gui);
void avdGuiStyleColorsLight(AVD_Gui* gui);

void avdGuiSeperator(AVD_Gui* gui);
void avdGuiSameLine(AVD_Gui* gui, float offsetX);
void avdGuiNewLine(AVD_Gui* gui);
void avdGuiSpacing(AVD_Gui* gui);
void avdGuiDummy(AVD_Gui* gui, AVD_Vector2 size);

void avdGuiText(AVD_Gui* gui, const char* format, ...);
void avdGuiTextColored(AVD_Gui* gui, AVD_Vector3 color, const char* format, ...);
void avdGuiTextDisabled(AVD_Gui* gui, const char* format, ...);
void avdGuiTextWrapped(AVD_Gui* gui, const char*format, ...);

AVD_Bool avdGuiButton(AVD_Gui* gui, const char* label, AVD_Vector2 size);
AVD_Bool avdGuiSmallButton(AVD_Gui* gui, const char* label);
AVD_Bool avdGuiInvisibleButton(AVD_Gui* gui, const char*label, AVD_Vector2 size);
AVD_Bool avdGuiArrowButton(AVD_Gui* gui, const char* id, AVD_GuiDir direction);
AVD_Bool avdGuiCheckbox(AVD_Gui* gui, const char* label, AVD_Bool* value);
AVD_Bool avdGuiRadioButton(AVD_Gui* gui, const char* label, AVD_Bool active);
void avdGuiProgressBar(AVD_Gui* gui, float fraction, AVD_Vector2 size, const char* overlay, ...);
void avdGuiBullet(AVD_Gui* gui);
AVD_Bool avdGuiTextLink(AVD_Gui* gui, const char* label, const char* url);

void avdGuiImage(AVD_Gui* gui, AVD_VulkanImageSubresource image, AVD_Vector2 size, AVD_Vector2 uvMin, AVD_Vector2 uvMax, AVD_Vector3 tintColor);
AVD_Bool avdGuiImageButton(AVD_Gui* gui, const char* label, AVD_VulkanImageSubresource image, AVD_Vector2 size, AVD_Vector2 uvMin, AVD_Vector2 uvMax, AVD_Vector3 tintColor);


AVD_Bool avdGuiDragFloat(AVD_Gui* gui, const char* label, float* value, float speed, float min, float max);
AVD_Bool avdGuiDragFloat2(AVD_Gui* gui, const char* label, AVD_Vector2* value, float speed, float min, float max);
AVD_Bool avdGuiDragFloat3(AVD_Gui* gui, const char* label, AVD_Vector3* value, float speed, float min, float max);
AVD_Bool avdGuiDragFloat4(AVD_Gui* gui, const char* label, AVD_Vector4* value, float speed, float min, float max);

AVD_Bool avdGuiDragInt(AVD_Gui* gui, const char* label, int* value, float speed, int min, int max);
AVD_Bool avdGuiDragInt2(AVD_Gui* gui, const char* label, AVD_Vector2* value, float speed, int min, int max);
AVD_Bool avdGuiDragInt3(AVD_Gui* gui, const char* label, AVD_Vector3* value, float speed, int min, int max);
AVD_Bool avdGuiDragInt4(AVD_Gui* gui, const char* label, AVD_Vector4* value, float speed, int min, int max);

AVD_Bool avdGuiSliderFloat(AVD_Gui* gui, const char* label, float* value, float min, float max);
AVD_Bool avdGuiSliderFloat2(AVD_Gui* gui, const char* label, AVD_Vector2* value, float min, float max);
AVD_Bool avdGuiSliderFloat3(AVD_Gui* gui, const char* label, AVD_Vector3* value, float min, float max);
AVD_Bool avdGuiSliderFloat4(AVD_Gui* gui, const char* label, AVD_Vector4* value, float min, float max);

AVD_Bool avdGuiSliderInt(AVD_Gui* gui, const char* label, int* value, int min, int max);
AVD_Bool avdGuiSliderInt2(AVD_Gui* gui, const char* label, AVD_Vector2* value, int min, int max);
AVD_Bool avdGuiSliderInt3(AVD_Gui* gui, const char* label, AVD_Vector3* value, int min, int max);
AVD_Bool avdGuiSliderInt4(AVD_Gui* gui, const char* label, AVD_Vector4* value, int min, int max);

AVD_Bool avdGuiInputText(AVD_Gui* gui, const char* label, char* buffer, size_t bufferSize);
AVD_Bool avdGuiInputTextMultiline(AVD_Gui* gui, const char* label, char* buffer, size_t bufferSize, AVD_Vector2 size);
AVD_Bool avdGuiInputInt(AVD_Gui* gui, const char* label, int* value, const char* format);
AVD_Bool avdGuiInputInt2(AVD_Gui* gui, const char* label, AVD_Vector2* value, const char* format);
AVD_Bool avdGuiInputInt3(AVD_Gui* gui, const char* label, AVD_Vector3* value, const char* format);
AVD_Bool avdGuiInputInt4(AVD_Gui* gui, const char* label, AVD_Vector4* value, const char* format);

AVD_Bool avdGuiInputFloat(AVD_Gui* gui, const char* label, float* value, const char* format);
AVD_Bool avdGuiInputFloat2(AVD_Gui* gui, const char* label, AVD_Vector2* value, const char* format);    
AVD_Bool avdGuiInputFloat3(AVD_Gui* gui, const char* label, AVD_Vector3* value, const char* format);
AVD_Bool avdGuiInputFloat4(AVD_Gui* gui, const char* label, AVD_Vector4* value, const char* format);

AVD_Bool avdGuiColorEdit3(AVD_Gui* gui, const char* label, AVD_Vector3* color);
AVD_Bool avdGuiColorEdit4(AVD_Gui* gui, const char* label, AVD_Vector4* color);
AVD_Bool avdGuiColorPicker3(AVD_Gui* gui, const char* label, AVD_Vector3* color);
AVD_Bool avdGuiColorPicker4(AVD_Gui* gui, const char* label, AVD_Vector4* color);

AVD_Bool avdGuiSelectable(AVD_Gui* gui, const char* label, AVD_Bool selected);

AVD_Bool avdGuiBeginTooltip(AVD_Gui* gui);
void avdGuiEndTooltip(AVD_Gui* gui);
AVD_Bool avdGuiSetTooltip(AVD_Gui* gui, const char* format, ...);

AVD_Bool avdGuiBeginPopup(AVD_Gui* gui, const char* id);
void avdGuiEndPopup(AVD_Gui* gui);
AVD_Bool avdGuiOpenPopup(AVD_Gui* gui, const char* id);
AVD_Bool avdGuiCloseCurrentPopup(AVD_Gui* gui);
AVD_Bool avdGuiIsPopupOpen(AVD_Gui* gui, const char* id);

AVD_Bool avdGuiBeginTabBar(AVD_Gui* gui, const char* id);
void avdGuiEndTabBar(AVD_Gui* gui);
AVD_Bool avdGuiBeginTabItem(AVD_Gui* gui, const char* label, AVD_Bool* open);
void avdGuiEndTabItem(AVD_Gui* gui);

AVD_Bool avdGuiIsItemHovered(AVD_Gui* gui);
AVD_Bool avdGuiIsItemActive(AVD_Gui* gui);
AVD_Bool avdGuiIsItemFocused(AVD_Gui* gui);
AVD_Bool avdGuiIsItemClicked(AVD_Gui* gui);
AVD_Bool avdGuiIsItemVisible(AVD_Gui* gui);
AVD_Bool avdGuiIsItemEdited(AVD_Gui* gui);
AVD_UInt32 avdGuiGetItemID(AVD_Gui* gui);
AVD_Vector2 avdGuiGetItemRectMin(AVD_Gui* gui);
AVD_Vector2 avdGuiGetItemRectMax(AVD_Gui* gui);
AVD_Vector2 avdGuiGetItemRectSize(AVD_Gui* gui);
AVD_Bool avdGuiIsRectVisible(AVD_Gui* gui, AVD_Vector2 rectMin, AVD_Vector2 rectMax);

AVD_Bool avdGuiWriteConfigToFile(AVD_Gui* gui, const char* filename);
AVD_Bool avdGuiLoadConfigFromFile(AVD_Gui* gui, const char* filename);
AVD_Bool avdGuiWriteConfigToMemory(AVD_Gui* gui, char* buffer, size_t bufferSize);
AVD_Bool avdGuiLoadConfigFromMemory(AVD_Gui* gui, const char* buffer, size_t bufferSize);

#endif // AVD_GUI_H