#ifndef AVD_GUI_H
#define AVD_GUI_H

#include "core/avd_input.h"
#include "core/avd_types.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_buffer.h"
#include "vulkan/avd_vulkan_image.h"
#include <stdint.h>

#ifndef AVD_GUI_MAX_COMPONENTS
#define AVD_GUI_MAX_COMPONENTS 1024
#endif

#ifndef AVD_GUI_MAX_DRAW_VERTICES
#define AVD_GUI_MAX_DRAW_VERTICES (AVD_GUI_MAX_COMPONENTS * 16)
#endif

typedef enum {
    AVD_GUI_COMPONENT_TYPE_NONE = 0,
    AVD_GUI_COMPONENT_TYPE_WINDOW,
    AVD_GUI_COMPONENT_TYPE_DUMMY,
    AVD_GUI_COMPONENT_TYPE_LAYOUT,
    AVD_GUI_COMPONENT_TYPE_LABEL,
    AVD_GUI_COMPONENT_TYPE_BUTTON,
    AVD_GUI_COMPONENT_TYPE_IMAGE,
    AVD_GUI_COMPONENT_TYPE_IMAGE_BUTTON,
    AVD_GUI_COMPONENT_TYPE_HYPERLINK,
    AVD_GUI_COMPONENT_TYPE_SLIDER,
    AVD_GUI_COMPONENT_TYPE_CHECKBOX,
    AVD_GUI_COMPONENT_TYPE_COLLAPSING_HEADER,
    AVD_GUI_COMPONENT_TYPE_DRAG,
    AVD_GUI_COMPONENT_TYPE_COUNT
} AVD_GuiComponentType;

typedef enum {
    AVD_GUI_WINDOW_TYPE_FLOATING = 0,
    AVD_GUI_WINDOW_TYPE_FULL_SCREEN,
    AVD_GUI_WINDOW_TYPE_LEFT_DOCKED,
    AVD_GUI_WINDOW_TYPE_RIGHT_DOCKED,
    AVD_GUI_WINDOW_TYPE_TOP_DOCKED,
    AVD_GUI_WINDOW_TYPE_BOTTOM_DOCKED,
    AVD_GUI_WINDOW_TYPE_COUNT
} AVD_GuiWindowType;

typedef enum {
    AVD_GUI_LAYOUT_ALIGN_START = 0,
    AVD_GUI_LAYOUT_ALIGN_CENTER,
    AVD_GUI_LAYOUT_ALIGN_END,
    AVD_GUI_LAYOUT_ALIGN_STRETCH,
    AVD_GUI_LAYOUT_ALIGN_COUNT
} AVD_GuiLayoutAlign;

typedef enum {
    AVD_GUI_LAYOUT_TYPE_NONE = 0,
    AVD_GUI_LAYOUT_TYPE_HORIZONTAL,
    AVD_GUI_LAYOUT_TYPE_VERTICAL,
    AVD_GUI_LAYOUT_TYPE_GRID,
    AVD_GUI_LAYOUT_TYPE_FLOAT,
    AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL,
    AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL,
    AVD_GUI_LAYOUT_TYPE_COUNT
} AVD_GuiLayout;

typedef enum {
    AVD_GUI_INTERACTION_NONE = 0,
    AVD_GUI_INTERACTION_DRAG,
    AVD_GUI_INTERACTION_RESIZE_LEFT,
    AVD_GUI_INTERACTION_RESIZE_RIGHT,
    AVD_GUI_INTERACTION_RESIZE_TOP,
    AVD_GUI_INTERACTION_RESIZE_BOTTOM,
    AVD_GUI_INTERACTION_RESIZE_TOP_LEFT,
    AVD_GUI_INTERACTION_RESIZE_TOP_RIGHT,
    AVD_GUI_INTERACTION_RESIZE_BOTTOM_LEFT,
    AVD_GUI_INTERACTION_RESIZE_BOTTOM_RIGHT,
    AVD_GUI_INTERACTION_COUNT
} AVD_GuiInteractionMode;

typedef AVD_UInt32 AVD_Color;

#define avdColorRgba(r, g, b, a) ((AVD_Color)(((uint8_t)((a) * 255.0f) << 24) | ((uint8_t)((b) * 255.0f) << 16) | ((uint8_t)((g) * 255.0f) << 8) | (uint8_t)((r) * 255.0f)))

typedef struct AVD_Gui AVD_Gui;
typedef struct AVD_GuiComponentHeader AVD_GuiComponentHeader;

typedef void (*AVD_GuiRenderRectsFunc)(AVD_Gui *gui, AVD_GuiComponentHeader *header);
typedef void (*AVD_GuiRenderTextFunc)(AVD_Gui *gui, AVD_GuiComponentHeader *header, VkCommandBuffer commandBuffer);

struct AVD_GuiComponentHeader {
    AVD_GuiComponentType type;
    AVD_UInt32 id;
    AVD_Double lastUpdateTime; // NOTE: not a good idea, make it integer based
    AVD_Vector2 pos;
    AVD_Vector2 size;
    AVD_RenderableText text;
    AVD_Color foregroundColor;
    AVD_Color backgroundColor;
    AVD_Vector2 clipRectPos;
    AVD_Vector2 clipRectSize;
    AVD_Bool isHovered;
    AVD_Bool isPressed;
    AVD_GuiRenderRectsFunc renderRects;
    AVD_GuiRenderTextFunc renderText;
};

typedef union AVD_GuiComponent AVD_GuiComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_GuiWindowType windowType;
    AVD_Vector2 minSize;
    AVD_Float titleBarHeight;
    AVD_GuiInteractionMode interactionMode;
    AVD_Vector2 dragOffset;
    AVD_GuiComponent *items[AVD_GUI_MAX_COMPONENTS];
    AVD_Int32 itemCount;
} AVD_GuiWindowComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_GuiLayout layoutType;
    AVD_GuiLayoutAlign horizontalAlign;
    AVD_GuiLayoutAlign verticalAlign;
    AVD_Float spacing;
    AVD_Vector2 cursor;
    AVD_Vector2 contentSize;
    AVD_Vector2 scrollOffset;
    AVD_Vector2 contentExtent;
    AVD_Bool autoContentWidth;
    AVD_Bool autoContentHeight;
    AVD_Bool verticalScrollbarDragging;
    AVD_Bool horizontalScrollbarDragging;
    AVD_GuiComponent *items[AVD_GUI_MAX_COMPONENTS];
    AVD_Int32 itemCount;
} AVD_GuiLayoutComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_Float charHeight;
    AVD_UInt32 textHash;
} AVD_GuiLabelComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_Float charHeight;
    AVD_UInt32 textHash;
} AVD_GuiButtonComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_VulkanImageSubresource *subresource;
    VkDescriptorSet descriptorSet;
} AVD_GuiImageComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_VulkanImageSubresource *subresource;
    VkDescriptorSet descriptorSet;
} AVD_GuiImageButtonComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_Float charHeight;
    AVD_UInt32 textHash;
    const char *url;
} AVD_GuiHyperlinkComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_Float value;
    AVD_Float minValue;
    AVD_Float maxValue;
    AVD_Bool isDragging;
} AVD_GuiSliderComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_Float charHeight;
    AVD_UInt32 textHash;
    AVD_Bool isChecked;
} AVD_GuiCheckboxComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_Float charHeight;
    AVD_UInt32 textHash;
    AVD_Bool open;
    AVD_GuiLayout childLayoutType;
    AVD_GuiLayoutAlign childHorizontalAlign;
    AVD_GuiLayoutAlign childVerticalAlign;
    AVD_Float childSpacing;
    AVD_GuiComponent *childLayout;
} AVD_GuiCollapsingHeaderComponent;

typedef struct {
    AVD_GuiComponentHeader header;
    AVD_Float charHeight;
    AVD_UInt32 textHash;
    AVD_Float value;
    AVD_Float minValue;
    AVD_Float maxValue;
    AVD_Float speed;
    AVD_Int32 precision;
    AVD_Bool isInteger;
    AVD_Bool isDragging;
} AVD_GuiDragComponent;

union AVD_GuiComponent {
    AVD_GuiComponentHeader header;
    AVD_GuiWindowComponent window;
    AVD_GuiLayoutComponent layout;
    AVD_GuiLabelComponent label;
    AVD_GuiButtonComponent button;
    AVD_GuiImageComponent image;
    AVD_GuiImageButtonComponent imageButton;
    AVD_GuiHyperlinkComponent hyperlink;
    AVD_GuiSliderComponent slider;
    AVD_GuiCheckboxComponent checkbox;
    AVD_GuiCollapsingHeaderComponent collapsingHeader;
    AVD_GuiDragComponent drag;
};

typedef struct {
    AVD_Vector2 mousePos;
    AVD_Vector2 mouseDelta;

    AVD_Bool mouseLeftPressed;
    AVD_Bool mouseRightPressed;
    AVD_Bool mouseMiddlePressed;
    AVD_Bool mouseLeftPressedThisFrame;
    AVD_Bool mouseLeftReleasedThisFrame;

    AVD_Float scrollDeltaX;
    AVD_Float scrollDeltaY;
    AVD_Bool keysPressed[GLFW_KEY_LAST + 1];
} AVD_GuiInputState;

typedef struct {
    uint16_t posX, posY;
    uint16_t sizeX, sizeY;
    uint16_t uv0X, uv0Y;
    uint16_t uv1X, uv1Y;
    uint16_t cornerRadius;
    uint16_t textured;
    uint32_t color;
} AVD_GuiVertexData;

typedef struct {
    AVD_Float padding;
    AVD_Float itemSpacing;
    AVD_Float cornerRadius;
    AVD_Float titleBarHeight;
    AVD_Color windowBgColor;
    AVD_Color windowBgFocusedColor;
    AVD_Color titleBarColor;
    AVD_Color textColor;
    AVD_Color buttonColor;
    AVD_Color buttonHoverColor;
    AVD_Color buttonPressColor;
    AVD_Color buttonTextColor;
    AVD_Color scrollTrackColor;
    AVD_Color scrollThumbColor;
    AVD_Color scrollThumbHoverColor;
    AVD_Color linkColor;
    AVD_Color linkHoverColor;
    AVD_Color sliderTrackColor;
    AVD_Color sliderThumbColor;
    AVD_Color sliderThumbHoverColor;
    AVD_Color sliderThumbActiveColor;
    AVD_Color headerColor;
    AVD_Color headerHoverColor;
    AVD_Color headerTextColor;
    AVD_Color headerArrowColor;
    AVD_Color dragBgColor;
    AVD_Color dragHoverColor;
    AVD_Color dragActiveColor;
    AVD_Color dragTextColor;
} AVD_GuiStyle;

struct AVD_Gui {
    AVD_Vulkan *vulkan;
    AVD_FontRenderer *fontRenderer;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    AVD_VulkanBuffer ssbo;

    AVD_GuiVertexData drawVertices[AVD_GUI_MAX_DRAW_VERTICES];
    AVD_UInt32 drawVertexCount;

    AVD_UInt32 idStack[AVD_GUI_MAX_COMPONENTS];
    AVD_Int32 idStackTop;

    AVD_GuiComponent *parentStack[AVD_GUI_MAX_COMPONENTS];
    AVD_Int32 parentStackTop;

    AVD_GuiStyle styleStack[AVD_GUI_MAX_COMPONENTS];
    AVD_Int32 styleStackTop;

    AVD_GuiInputState inputState;

    AVD_Vector2 screenSize;

    AVD_UInt32 idSeed;
    AVD_Bool interacting;

    AVD_GuiComponent components[AVD_GUI_MAX_COMPONENTS];
    AVD_GuiComponent *activeLayout;
    AVD_GuiWindowComponent *window;

    AVD_Bool windowFocused;
};

AVD_GuiStyle avdGuiStyleDefault(void);

bool avdGuiCreate(
    AVD_Gui *gui,
    AVD_Vulkan *vulkan,
    AVD_FontManager *fontManager,
    AVD_FontRenderer *fontRenderer,
    VkRenderPass renderPass,
    AVD_Vector2 screenSize);
void avdGuiDestroy(AVD_Gui *gui);

void avdGuiPushStyle(AVD_Gui *gui, AVD_GuiStyle style);
void avdGuiPopStyle(AVD_Gui *gui);

bool avdGuiPushEvent(AVD_Gui *gui, AVD_InputEvent *event);
bool avdGuiBegin(
    AVD_Gui *gui,
    AVD_Vector2 minSize,
    AVD_GuiWindowType windowType);
void avdGuiEnd(AVD_Gui *gui);
bool avdGuiRender(AVD_Gui *gui, VkCommandBuffer commandBuffer);

bool avdGuiBeginLayout(
    AVD_Gui *gui,
    AVD_GuiLayout layoutType,
    AVD_GuiLayoutAlign horizontalAlign,
    AVD_GuiLayoutAlign verticalAlign,
    AVD_Float spacing,
    const char *label);
bool avdGuiBeginScrollableLayout(
    AVD_Gui *gui,
    AVD_GuiLayout layoutType,
    AVD_GuiLayoutAlign horizontalAlign,
    AVD_GuiLayoutAlign verticalAlign,
    AVD_Float spacing,
    AVD_Vector2 contentSize,
    const char *label);
void avdGuiEndLayout(AVD_Gui *gui);

void avdGuiDummy(AVD_Gui *gui, AVD_Vector2 size, AVD_Color color, const char *label);
bool avdGuiLabel(AVD_Gui *gui, const char *text, const char *fontName, AVD_Float charHeight, const char *label);
bool avdGuiButton(AVD_Gui *gui, const char *label, const char *fontName, AVD_Float charHeight, AVD_Vector2 size);
void avdGuiImage(AVD_Gui *gui, AVD_VulkanImageSubresource *subresource, AVD_Vector2 size, const char *label);
bool avdGuiImageButton(AVD_Gui *gui, const char *label, AVD_VulkanImageSubresource *subresource, AVD_Vector2 size);
bool avdGuiHyperlink(AVD_Gui *gui, const char *text, const char *url, const char *fontName, AVD_Float charHeight, const char *label);
bool avdGuiSlider(AVD_Gui *gui, const char *label, AVD_Float *value, AVD_Float minValue, AVD_Float maxValue, AVD_Vector2 size);
bool avdGuiCheckbox(AVD_Gui *gui, const char *label, AVD_Bool *checked, const char *fontName, AVD_Float charHeight);
bool avdGuiDragFloat(AVD_Gui *gui, const char *label, AVD_Float *value, AVD_Float speed, AVD_Float minValue, AVD_Float maxValue, AVD_Int32 precision, const char *fontName, AVD_Float charHeight, AVD_Vector2 size);
bool avdGuiDragInt(AVD_Gui *gui, const char *label, AVD_Int32 *value, AVD_Float speed, AVD_Int32 minValue, AVD_Int32 maxValue, const char *fontName, AVD_Float charHeight, AVD_Vector2 size);

bool avdGuiBeginCollapsingHeader(AVD_Gui *gui, const char *label, const char *fontName, AVD_Float charHeight, AVD_Float headerHeight);
void avdGuiEndCollapsingHeader(AVD_Gui *gui);

bool avdGuiPushId(AVD_Gui *gui, const char *strId);
void avdGuiPopId(AVD_Gui *gui);

AVD_Bool avdGuiIsItemHovered(AVD_Gui *gui);
AVD_Bool avdGuiIsItemClicked(AVD_Gui *gui);
AVD_Bool avdGuiIsItemPressed(AVD_Gui *gui);

AVD_Vector2 avdGuiGetAvailableSize(AVD_Gui *gui);
AVD_Vector2 avdGuiGetContentSize(AVD_Gui *gui);
AVD_Vector2 avdGuiGetContentArea(AVD_Gui *gui);

#endif // AVD_GUI_H