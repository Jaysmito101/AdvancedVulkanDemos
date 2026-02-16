#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"
#include <string.h>

static void __avdGuiButtonRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    avdGuiPushRect(gui, header->pos, header->size, header->backgroundColor, header->clipRectPos, header->clipRectSize);
}

static void __avdGuiButtonRenderText(AVD_Gui *gui, AVD_GuiComponentHeader *header, VkCommandBuffer commandBuffer)
{
    AVD_GuiButtonComponent *btn = (AVD_GuiButtonComponent *)header;
    if (!btn->header.text.initialized) {
        return;
    }

    AVD_Color fc = btn->header.foregroundColor;
    AVD_Float cr = ((fc >> 0) & 0xFF) / 255.0f;
    AVD_Float cg = ((fc >> 8) & 0xFF) / 255.0f;
    AVD_Float cb = ((fc >> 16) & 0xFF) / 255.0f;
    AVD_Float ca = ((fc >> 24) & 0xFF) / 255.0f;

    AVD_Float textW = 0.0f, textH = 0.0f;
    avdRenderableTextGetSize(&btn->header.text, &textW, &textH);

    AVD_Float textX = btn->header.pos.x + (btn->header.size.x - textW) * 0.5f;
    AVD_Float textY = btn->header.pos.y + (btn->header.size.y + textH) * 0.5f;

    VkRect2D btnScissor = avdGuiScissorFromRect(gui, btn->header.clipRectPos, btn->header.clipRectSize);
    vkCmdSetScissor(commandBuffer, 0, 1, &btnScissor);

    avdRenderText(
        gui->vulkan,
        gui->fontRenderer,
        &btn->header.text,
        commandBuffer,
        textX, textY,
        1.0f,
        cr, cg, cb, ca,
        (uint32_t)gui->screenSize.x,
        (uint32_t)gui->screenSize.y);
}

bool avdGuiButton(
    AVD_Gui *gui,
    const char *label,
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

    bool clicked = false;

    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_BUTTON, idStr);
    AVD_GuiComponent *buttonComp   = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_BUTTON, resolvedId);
    AVD_GuiButtonComponent *btn    = &buttonComp->button;

    if (btn->charHeight <= 0.0f) {
        btn->charHeight = charHeight > 0.0f ? charHeight : 20.0f;
    }

    if (!btn->header.text.initialized) {
        AVD_CHECK(avdRenderableTextCreate(
            &btn->header.text,
            gui->fontRenderer,
            gui->vulkan,
            fontName,
            displayText,
            btn->charHeight));
        btn->textHash = avdHashString(displayText);
    } else {
        AVD_UInt32 newHash = avdHashString(displayText);
        if (btn->textHash != newHash) {
            AVD_CHECK(avdRenderableTextUpdate(
                &btn->header.text,
                gui->fontRenderer,
                gui->vulkan,
                displayText));
            btn->textHash = newHash;
        }
    }

    AVD_Float textWidth = 0.0f, textHeight = 0.0f;
    avdRenderableTextGetSize(&btn->header.text, &textWidth, &textHeight);

    AVD_Vector2 resolvedSize = avdVec2(
        size.x > 0.0f ? size.x : (textWidth + style->padding * 2.0f),
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

    btn->header.pos  = pos;
    btn->header.size = resolvedSize;

    btn->header.clipRectPos     = layout->header.clipRectPos;
    btn->header.clipRectSize    = layout->header.clipRectSize;
    btn->header.foregroundColor = style->buttonTextColor;

    btn->header.isHovered = avdVec2IntersectRect(gui->inputState.mousePos, btn->header.pos, btn->header.size);

    if (gui->inputState.mouseLeftPressedThisFrame && btn->header.isHovered) {
        btn->header.isPressed = true;
    }

    if (gui->inputState.mouseLeftReleasedThisFrame) {
        if (btn->header.isPressed && btn->header.isHovered) {
            clicked = true;
        }
        btn->header.isPressed = false;
    }

    if (btn->header.isPressed) {
        btn->header.backgroundColor = style->buttonPressColor;
    } else if (btn->header.isHovered) {
        btn->header.backgroundColor = style->buttonHoverColor;
    } else {
        btn->header.backgroundColor = style->buttonColor;
    }

    btn->header.renderRects = __avdGuiButtonRenderRects;
    btn->header.renderText  = __avdGuiButtonRenderText;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = buttonComp;
    return clicked;
}
