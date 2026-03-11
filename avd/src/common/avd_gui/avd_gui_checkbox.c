#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"
#include <string.h>

static void __avdGuiCheckboxRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    AVD_GuiCheckboxComponent *checkbox = (AVD_GuiCheckboxComponent *)header;
    AVD_GuiStyle *style                = avdGuiCurrentStyle(gui);

    AVD_Float boxSize      = header->size.y;
    AVD_Float dim          = checkbox->charHeight;
    AVD_Vector2 boxPos     = avdVec2(header->pos.x, header->pos.y + (header->size.y - dim) * 0.5f);
    AVD_Vector2 boxSizeVec = avdVec2(dim, dim);

    AVD_Color bgColor = header->backgroundColor;
    avdGuiPushRect(
        gui,
        boxPos,
        boxSizeVec,
        bgColor,
        header->clipRectPos,
        header->clipRectSize);

    if (checkbox->isChecked) {
        AVD_Float padding     = dim * 0.2f;
        AVD_Vector2 checkPos  = avdVec2(boxPos.x + padding, boxPos.y + padding);
        AVD_Vector2 checkSize = avdVec2(dim - padding * 2.0f, dim - padding * 2.0f);

        avdGuiPushRect(
            gui,
            checkPos,
            checkSize,
            style->buttonTextColor,
            header->clipRectPos,
            header->clipRectSize);
    }
}

static void __avdGuiCheckboxRenderText(AVD_Gui *gui, AVD_GuiComponentHeader *header, VkCommandBuffer commandBuffer)
{
    AVD_GuiCheckboxComponent *checkbox = (AVD_GuiCheckboxComponent *)header;
    if (!checkbox->header.text.initialized) {
        return;
    }

    AVD_Color fc = checkbox->header.foregroundColor;
    AVD_Float cr = ((fc >> 0) & 0xFF) / 255.0f;
    AVD_Float cg = ((fc >> 8) & 0xFF) / 255.0f;
    AVD_Float cb = ((fc >> 16) & 0xFF) / 255.0f;
    AVD_Float ca = ((fc >> 24) & 0xFF) / 255.0f;

    AVD_Float textW = 0.0f, textH = 0.0f;
    avdRenderableTextGetSize(&checkbox->header.text, &textW, &textH);

    AVD_Float boxSize   = checkbox->header.size.y;
    AVD_GuiStyle *style = avdGuiCurrentStyle(gui);

    AVD_Float dim   = checkbox->charHeight;
    AVD_Float textX = checkbox->header.pos.x + dim + style->itemSpacing;
    AVD_Float textY = checkbox->header.pos.y + (checkbox->header.size.y + textH) * 0.5f - 4.0f;

    VkRect2D scissor = avdGuiScissorFromRect(gui, checkbox->header.clipRectPos, checkbox->header.clipRectSize);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    avdRenderText(
        gui->vulkan,
        gui->fontRenderer,
        &checkbox->header.text,
        commandBuffer,
        textX, textY,
        1.0f,
        cr, cg, cb, ca,
        (uint32_t)gui->screenSize.x,
        (uint32_t)gui->screenSize.y);
}

bool avdGuiCheckbox(
    AVD_Gui *gui,
    const char *label,
    AVD_Bool *checked,
    const char *fontName,
    AVD_Float charHeight)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);
    AVD_ASSERT(label != NULL);
    AVD_ASSERT(checked != NULL);
    AVD_ASSERT(fontName != NULL);

    char displayBuffer[256];
    const char *displayText;
    const char *idStr;
    avdGuiSplitLabel(label, displayBuffer, sizeof(displayBuffer), &displayText, &idStr);

    bool valueChanged = false;

    AVD_GuiStyle *style                = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout     = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId              = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_CHECKBOX, idStr);
    AVD_GuiComponent *comp             = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_CHECKBOX, resolvedId);
    AVD_GuiCheckboxComponent *checkbox = &comp->checkbox;

    checkbox->charHeight = charHeight > 0.0f ? charHeight : 20.0f;
    checkbox->isChecked  = *checked;

    if (!checkbox->header.text.initialized) {
        AVD_CHECK(avdRenderableTextCreate(
            &checkbox->header.text,
            gui->fontRenderer,
            gui->vulkan,
            fontName,
            displayText,
            checkbox->charHeight));
        checkbox->textHash = avdHashString(displayText);
    } else {
        AVD_UInt32 newHash = avdHashString(displayText);
        if (checkbox->textHash != newHash) {
            AVD_CHECK(avdRenderableTextUpdate(
                &checkbox->header.text,
                gui->fontRenderer,
                gui->vulkan,
                displayText));
            checkbox->textHash = newHash;
        }
    }

    AVD_Float textWidth = 0.0f, textHeight = 0.0f;
    avdRenderableTextGetSize(&checkbox->header.text, &textWidth, &textHeight);

    AVD_Float boxSize     = checkbox->charHeight;
    AVD_Float totalWidth  = boxSize + style->itemSpacing + textWidth;
    AVD_Float totalHeight = AVD_MAX(boxSize, textHeight);

    totalWidth += style->padding * 2.0f;
    totalHeight += style->padding * 2.0f;

    AVD_Vector2 resolvedSize = avdVec2(totalWidth, totalHeight);

    AVD_Vector2 pos     = avdVec2Zero();
    AVD_Vector2 maxSize = avdVec2Zero();
    avdGuiResolveComponentPosition(layout, resolvedSize, style, &pos, &maxSize);

    avdGuiApplyItemAlignment(layout, resolvedSize, &pos, &maxSize);

    checkbox->header.pos  = pos;
    checkbox->header.size = resolvedSize;

    checkbox->header.clipRectPos     = layout->header.clipRectPos;
    checkbox->header.clipRectSize    = layout->header.clipRectSize;
    checkbox->header.foregroundColor = style->textColor;

    checkbox->header.isHovered = avdVec2IntersectRect(gui->inputState.mousePos, checkbox->header.pos, checkbox->header.size);

    if (gui->inputState.mouseLeftPressedThisFrame && checkbox->header.isHovered) {
        checkbox->header.isPressed = true;
    }

    if (gui->inputState.mouseLeftReleasedThisFrame) {
        if (checkbox->header.isPressed && checkbox->header.isHovered) {
            *checked            = !(*checked);
            checkbox->isChecked = *checked;
            valueChanged        = true;
        }
        checkbox->header.isPressed = false;
    }

    if (checkbox->header.isPressed) {
        checkbox->header.backgroundColor = style->buttonPressColor;
    } else if (checkbox->header.isHovered) {
        checkbox->header.backgroundColor = style->buttonHoverColor;
    } else {
        checkbox->header.backgroundColor = style->buttonColor;
    }

    checkbox->header.renderRects = __avdGuiCheckboxRenderRects;
    checkbox->header.renderText  = __avdGuiCheckboxRenderText;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = comp;

    return valueChanged;
}
