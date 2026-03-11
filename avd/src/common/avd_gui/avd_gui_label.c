#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"

static void __avdGuiLabelRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    (void)gui;
    (void)header;
    // Labels have no background rect
}

static void __avdGuiLabelRenderText(AVD_Gui *gui, AVD_GuiComponentHeader *header, VkCommandBuffer commandBuffer)
{
    AVD_GuiLabelComponent *lbl = (AVD_GuiLabelComponent *)header;
    if (!lbl->header.text.initialized) {
        return;
    }

    AVD_Color fc = lbl->header.foregroundColor;
    AVD_Float cr = ((fc >> 0) & 0xFF) / 255.0f;
    AVD_Float cg = ((fc >> 8) & 0xFF) / 255.0f;
    AVD_Float cb = ((fc >> 16) & 0xFF) / 255.0f;
    AVD_Float ca = ((fc >> 24) & 0xFF) / 255.0f;

    VkRect2D labelScissor = avdGuiScissorFromRect(gui, lbl->header.clipRectPos, lbl->header.clipRectSize);
    vkCmdSetScissor(commandBuffer, 0, 1, &labelScissor);

    avdRenderText(
        gui->vulkan,
        gui->fontRenderer,
        &lbl->header.text,
        commandBuffer,
        lbl->header.pos.x,
        lbl->header.pos.y + lbl->header.size.y,
        1.0f,
        cr, cg, cb, ca,
        (uint32_t)gui->screenSize.x,
        (uint32_t)gui->screenSize.y);
}

bool avdGuiLabel(
    AVD_Gui *gui,
    const char *text,
    const char *fontName,
    AVD_Float charHeight,
    const char *label)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);
    AVD_ASSERT(text != NULL);
    AVD_ASSERT(fontName != NULL);

    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_LABEL, label);
    AVD_GuiComponent *labelComp    = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_LABEL, resolvedId);
    AVD_GuiLabelComponent *lbl     = &labelComp->label;

    if (lbl->charHeight <= 0.0f) {
        lbl->charHeight = charHeight > 0.0f ? charHeight : 24.0f;
    }

    if (!lbl->header.text.initialized) {
        AVD_CHECK(avdRenderableTextCreate(
            &lbl->header.text,
            gui->fontRenderer,
            gui->vulkan,
            fontName,
            text,
            lbl->charHeight));
        lbl->textHash = avdHashString(text);
    } else {
        AVD_UInt32 newHash = avdHashString(text);
        if (lbl->textHash != newHash) {
            AVD_CHECK(avdRenderableTextUpdate(
                &lbl->header.text,
                gui->fontRenderer,
                gui->vulkan,
                text));
            lbl->textHash = newHash;
        }
    }

    avdRenderableTextGetSize(&lbl->header.text, &lbl->header.size.x, &lbl->header.size.y);

    AVD_Vector2 pos     = avdVec2Zero();
    AVD_Vector2 maxSize = avdVec2Zero();
    avdGuiResolveComponentPosition(layout, lbl->header.size, style, &pos, &maxSize);
    avdGuiApplyItemAlignment(layout, lbl->header.size, &pos, &maxSize);

    lbl->header.pos             = pos;
    lbl->header.clipRectPos     = layout->header.clipRectPos;
    lbl->header.clipRectSize    = layout->header.clipRectSize;
    lbl->header.foregroundColor = style->textColor;
    lbl->header.isHovered       = avdVec2IntersectRect(gui->inputState.mousePos, pos, lbl->header.size);
    lbl->header.isPressed       = lbl->header.isHovered && gui->inputState.mouseLeftPressed;
    lbl->header.renderRects     = __avdGuiLabelRenderRects;
    lbl->header.renderText      = __avdGuiLabelRenderText;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = labelComp;
    return true;
}
