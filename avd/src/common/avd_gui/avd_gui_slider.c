#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "math/avd_vector_non_simd.h"
#include <string.h>

static void __avdGuiSliderRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    AVD_GuiSliderComponent *slider = (AVD_GuiSliderComponent *)header;
    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);

    avdGuiPushRect(
        gui,
        slider->header.pos,
        slider->header.size,
        style->sliderTrackColor,
        slider->header.clipRectPos,
        slider->header.clipRectSize);

    AVD_Float normalizedValue = (slider->value - slider->minValue) / (slider->maxValue - slider->minValue);
    AVD_Float thumbRadius     = slider->header.size.y * 0.5f;
    AVD_Float thumbX          = slider->header.pos.x + thumbRadius + normalizedValue * (slider->header.size.x - thumbRadius * 2.0f);
    AVD_Float thumbY          = slider->header.pos.y + slider->header.size.y * 0.5f;

    AVD_Vector2 thumbPos  = avdVec2(thumbX - thumbRadius, thumbY - thumbRadius);
    AVD_Vector2 thumbSize = avdVec2(thumbRadius * 2.0f, thumbRadius * 2.0f);

    AVD_Color thumbColor = slider->header.backgroundColor;
    avdGuiPushRect(
        gui,
        thumbPos,
        thumbSize,
        thumbColor,
        slider->header.clipRectPos,
        slider->header.clipRectSize);
}

bool avdGuiSlider(
    AVD_Gui *gui,
    const char *label,
    AVD_Float *value,
    AVD_Float minValue,
    AVD_Float maxValue,
    AVD_Vector2 size)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);
    AVD_ASSERT(label != NULL);
    AVD_ASSERT(value != NULL);

    char displayBuffer[256];
    const char *displayText;
    const char *idStr;
    avdGuiSplitLabel(label, displayBuffer, sizeof(displayBuffer), &displayText, &idStr);

    bool valueChanged = false;

    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_SLIDER, idStr);
    AVD_GuiComponent *sliderComp   = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_SLIDER, resolvedId);
    AVD_GuiSliderComponent *slider = &sliderComp->slider;

    slider->minValue = minValue;
    slider->maxValue = maxValue;

    if (slider->value < minValue)
        slider->value = minValue;
    if (slider->value > maxValue)
        slider->value = maxValue;

    AVD_Vector2 resolvedSize = avdVec2(
        size.x > 0.0f ? size.x : 200.0f,
        size.y > 0.0f ? size.y : 20.0f);

    AVD_Vector2 pos     = avdVec2Zero();
    AVD_Vector2 maxSize = avdVec2Zero();
    avdGuiResolveComponentPosition(layout, resolvedSize, style, &pos, &maxSize);

    if (size.x == 0.0f) {
        resolvedSize.x = maxSize.x;
    }
    if (size.y == 0.0f) {
        resolvedSize.y = maxSize.y;
    }

    avdGuiApplyItemAlignment(layout, resolvedSize, &pos, &maxSize);

    slider->header.pos  = pos;
    slider->header.size = resolvedSize;

    slider->header.clipRectPos  = layout->header.clipRectPos;
    slider->header.clipRectSize = layout->header.clipRectSize;

    AVD_Float thumbRadius     = slider->header.size.y * 0.5f;
    AVD_Float normalizedValue = (slider->value - slider->minValue) / (slider->maxValue - slider->minValue);
    AVD_Float thumbX          = slider->header.pos.x + thumbRadius + normalizedValue * (slider->header.size.x - thumbRadius * 2.0f);
    AVD_Float thumbY          = slider->header.pos.y + slider->header.size.y * 0.5f;

    AVD_Vector2 thumbPos  = avdVec2(thumbX - thumbRadius, thumbY - thumbRadius);
    AVD_Vector2 thumbSize = avdVec2(thumbRadius * 2.0f, thumbRadius * 2.0f);

    slider->header.isHovered = avdVec2IntersectRect(gui->inputState.mousePos, slider->header.pos, slider->header.size);
    AVD_Bool thumbHovered    = avdVec2IntersectRect(gui->inputState.mousePos, thumbPos, thumbSize);

    if (gui->inputState.mouseLeftPressedThisFrame && (slider->header.isHovered || thumbHovered)) {
        slider->isDragging = true;
        gui->interacting   = true;
    }

    if (gui->inputState.mouseLeftReleasedThisFrame) {
        slider->isDragging = false;
    }

    if (slider->isDragging) {
        AVD_Float mouseX        = gui->inputState.mousePos.x;
        AVD_Float sliderStart   = slider->header.pos.x + thumbRadius;
        AVD_Float sliderEnd     = slider->header.pos.x + slider->header.size.x - thumbRadius;
        AVD_Float newNormalized = (mouseX - sliderStart) / (sliderEnd - sliderStart);

        if (newNormalized < 0.0f)
            newNormalized = 0.0f;
        if (newNormalized > 1.0f)
            newNormalized = 1.0f;

        AVD_Float newValue = slider->minValue + newNormalized * (slider->maxValue - slider->minValue);
        if (newValue != slider->value) {
            slider->value = newValue;
            valueChanged  = true;
        }
    }

    if (slider->isDragging) {
        slider->header.backgroundColor = style->sliderThumbActiveColor;
    } else if (thumbHovered) {
        slider->header.backgroundColor = style->sliderThumbHoverColor;
    } else {
        slider->header.backgroundColor = style->sliderThumbColor;
    }

    slider->header.renderRects = __avdGuiSliderRenderRects;
    slider->header.renderText  = NULL;

    *value = slider->value;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = sliderComp;

    return valueChanged;
}
