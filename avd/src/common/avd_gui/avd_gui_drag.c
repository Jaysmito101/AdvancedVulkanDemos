#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"
#include <stdio.h>
#include <string.h>

static void __avdGuiDragRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    avdGuiPushRect(gui, header->pos, header->size, header->backgroundColor, header->clipRectPos, header->clipRectSize);
}

static void __avdGuiDragRenderText(AVD_Gui *gui, AVD_GuiComponentHeader *header, VkCommandBuffer commandBuffer)
{
    AVD_GuiDragComponent *drag = (AVD_GuiDragComponent *)header;
    if (!drag->header.text.initialized) {
        return;
    }

    AVD_Color fc = drag->header.foregroundColor;
    AVD_Float cr = ((fc >> 0) & 0xFF) / 255.0f;
    AVD_Float cg = ((fc >> 8) & 0xFF) / 255.0f;
    AVD_Float cb = ((fc >> 16) & 0xFF) / 255.0f;
    AVD_Float ca = ((fc >> 24) & 0xFF) / 255.0f;

    AVD_Float textW = 0.0f, textH = 0.0f;
    avdRenderableTextGetSize(&drag->header.text, &textW, &textH);

    AVD_Float textX = drag->header.pos.x + (drag->header.size.x - textW) * 0.5f;
    AVD_Float textY = drag->header.pos.y + (drag->header.size.y + textH) * 0.5f;

    VkRect2D scissor = avdGuiScissorFromRect(gui, drag->header.clipRectPos, drag->header.clipRectSize);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    avdRenderText(
        gui->vulkan,
        gui->fontRenderer,
        &drag->header.text,
        commandBuffer,
        textX, textY,
        1.0f,
        cr, cg, cb, ca,
        (uint32_t)gui->screenSize.x,
        (uint32_t)gui->screenSize.y);
}

static bool __avdGuiDragInternal(
    AVD_Gui *gui,
    const char *label,
    void *value,
    AVD_Float speed,
    AVD_Float minValue,
    AVD_Float maxValue,
    AVD_Int32 precision,
    AVD_Bool isInteger,
    const char *fontName,
    AVD_Float charHeight,
    AVD_Vector2 size)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);
    AVD_ASSERT(label != NULL);
    AVD_ASSERT(fontName != NULL);

    char displayBuffer[256];
    const char *displayText = label;
    const char *idStr       = label;
    const char *separator   = strstr(label, "##");
    if (separator != NULL) {
        size_t len = (size_t)(separator - label);
        if (len >= sizeof(displayBuffer)) {
            len = sizeof(displayBuffer) - 1;
        }
        memcpy(displayBuffer, label, len);
        displayBuffer[len] = '\0';
        displayText        = displayBuffer;
        idStr              = separator + 2;
    }

    (void)displayText;

    bool valueChanged = false;

    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_DRAG, idStr);
    AVD_GuiComponent *dragComp     = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_DRAG, resolvedId);
    AVD_GuiDragComponent *drag     = &dragComp->drag;

    drag->minValue  = minValue;
    drag->maxValue  = maxValue;
    drag->speed     = speed > 0.0f ? speed : 1.0f;
    drag->precision = precision >= 0 ? precision : 2;
    drag->isInteger = isInteger;

    drag->value = AVD_CLAMP(drag->value, minValue, maxValue);

    if (drag->charHeight <= 0.0f) {
        drag->charHeight = charHeight > 0.0f ? charHeight : 20.0f;
    }

    char valueStr[64];
    if (isInteger) {
        snprintf(valueStr, sizeof(valueStr), "%d", (int)drag->value);
    } else {
        snprintf(valueStr, sizeof(valueStr), "%.*f", drag->precision, drag->value);
    }

    if (!drag->header.text.initialized) {
        AVD_CHECK(avdRenderableTextCreate(
            &drag->header.text,
            gui->fontRenderer,
            gui->vulkan,
            fontName,
            valueStr,
            drag->charHeight));
        drag->textHash = avdHashString(valueStr);
    } else {
        AVD_UInt32 newHash = avdHashString(valueStr);
        if (drag->textHash != newHash) {
            AVD_CHECK(avdRenderableTextUpdate(
                &drag->header.text,
                gui->fontRenderer,
                gui->vulkan,
                valueStr));
            drag->textHash = newHash;
        }
    }

    AVD_Float textWidth = 0.0f, textHeight = 0.0f;
    avdRenderableTextGetSize(&drag->header.text, &textWidth, &textHeight);

    AVD_Vector2 resolvedSize = avdVec2(
        size.x > 0.0f ? size.x : (textWidth + style->padding * 4.0f),
        size.y > 0.0f ? size.y : (textHeight + style->padding * 2.0f));

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

    drag->header.pos  = pos;
    drag->header.size = resolvedSize;

    drag->header.clipRectPos  = layout->header.clipRectPos;
    drag->header.clipRectSize = layout->header.clipRectSize;

    drag->header.isHovered = avdVec2IntersectRect(gui->inputState.mousePos, drag->header.pos, drag->header.size);

    if (gui->inputState.mouseLeftPressedThisFrame && drag->header.isHovered) {
        drag->isDragging = true;
        gui->interacting = true;
    }

    if (gui->inputState.mouseLeftReleasedThisFrame) {
        drag->isDragging = false;
    }

    if (drag->isDragging) {
        AVD_Float delta    = gui->inputState.mouseDelta.x * drag->speed;
        AVD_Float newValue = AVD_CLAMP(drag->value + delta, minValue, maxValue);

        if (isInteger) {
            newValue = (AVD_Float)((AVD_Int32)newValue);
        }

        if (newValue != drag->value) {
            drag->value  = newValue;
            valueChanged = true;
        }
    }

    if (drag->isDragging) {
        drag->header.backgroundColor = style->dragActiveColor;
    } else if (drag->header.isHovered) {
        drag->header.backgroundColor = style->dragHoverColor;
    } else {
        drag->header.backgroundColor = style->dragBgColor;
    }

    drag->header.foregroundColor = style->dragTextColor;
    drag->header.renderRects     = __avdGuiDragRenderRects;
    drag->header.renderText      = __avdGuiDragRenderText;

    if (isInteger) {
        *(AVD_Int32 *)value = (AVD_Int32)drag->value;
    } else {
        *(AVD_Float *)value = drag->value;
    }

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = dragComp;

    return valueChanged;
}

bool avdGuiDragFloat(
    AVD_Gui *gui,
    const char *label,
    AVD_Float *value,
    AVD_Float speed,
    AVD_Float minValue,
    AVD_Float maxValue,
    AVD_Int32 precision,
    const char *fontName,
    AVD_Float charHeight,
    AVD_Vector2 size)
{
    AVD_ASSERT(value != NULL);
    return __avdGuiDragInternal(gui, label, value, speed, minValue, maxValue, precision, false, fontName, charHeight, size);
}

bool avdGuiDragInt(
    AVD_Gui *gui,
    const char *label,
    AVD_Int32 *value,
    AVD_Float speed,
    AVD_Int32 minValue,
    AVD_Int32 maxValue,
    const char *fontName,
    AVD_Float charHeight,
    AVD_Vector2 size)
{
    AVD_ASSERT(value != NULL);
    return __avdGuiDragInternal(gui, label, value, speed, (AVD_Float)minValue, (AVD_Float)maxValue, 0, true, fontName, charHeight, size);
}
