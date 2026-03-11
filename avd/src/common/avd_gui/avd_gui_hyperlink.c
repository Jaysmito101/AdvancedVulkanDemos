#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"

static void __avdGuiHyperlinkRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    AVD_GuiHyperlinkComponent *link = (AVD_GuiHyperlinkComponent *)header;

    if (link->header.isHovered) {
        AVD_Float underlineY      = link->header.pos.y + link->header.size.y;
        AVD_Float underlineHeight = 2.0f;
        AVD_Vector2 underlinePos  = avdVec2(link->header.pos.x, underlineY);
        AVD_Vector2 underlineSize = avdVec2(link->header.size.x, underlineHeight);

        avdGuiPushRect(
            gui,
            underlinePos,
            underlineSize,
            link->header.foregroundColor,
            link->header.clipRectPos,
            link->header.clipRectSize);
    }
}

static void __avdGuiHyperlinkRenderText(AVD_Gui *gui, AVD_GuiComponentHeader *header, VkCommandBuffer commandBuffer)
{
    AVD_GuiHyperlinkComponent *link = (AVD_GuiHyperlinkComponent *)header;
    if (!link->header.text.initialized) {
        return;
    }

    AVD_Color fc = link->header.foregroundColor;
    AVD_Float cr = ((fc >> 0) & 0xFF) / 255.0f;
    AVD_Float cg = ((fc >> 8) & 0xFF) / 255.0f;
    AVD_Float cb = ((fc >> 16) & 0xFF) / 255.0f;
    AVD_Float ca = ((fc >> 24) & 0xFF) / 255.0f;

    VkRect2D linkScissor = avdGuiScissorFromRect(gui, link->header.clipRectPos, link->header.clipRectSize);
    vkCmdSetScissor(commandBuffer, 0, 1, &linkScissor);

    avdRenderText(
        gui->vulkan,
        gui->fontRenderer,
        &link->header.text,
        commandBuffer,
        link->header.pos.x,
        link->header.pos.y + link->header.size.y,
        1.0f,
        cr, cg, cb, ca,
        (uint32_t)gui->screenSize.x,
        (uint32_t)gui->screenSize.y);
}

bool avdGuiHyperlink(
    AVD_Gui *gui,
    const char *text,
    const char *url,
    const char *fontName,
    AVD_Float charHeight,
    const char *label)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);
    AVD_ASSERT(text != NULL);
    AVD_ASSERT(url != NULL);
    AVD_ASSERT(fontName != NULL);

    AVD_GuiStyle *style             = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout  = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId           = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_HYPERLINK, label ? label : text);
    AVD_GuiComponent *linkComp      = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_HYPERLINK, resolvedId);
    AVD_GuiHyperlinkComponent *link = &linkComp->hyperlink;

    if (link->charHeight <= 0.0f) {
        link->charHeight = charHeight > 0.0f ? charHeight : 20.0f;
    }

    if (!link->header.text.initialized) {
        AVD_CHECK(avdRenderableTextCreate(
            &link->header.text,
            gui->fontRenderer,
            gui->vulkan,
            fontName,
            text,
            link->charHeight));
        link->textHash = avdHashString(text);
    } else {
        AVD_UInt32 newHash = avdHashString(text);
        if (link->textHash != newHash) {
            AVD_CHECK(avdRenderableTextUpdate(
                &link->header.text,
                gui->fontRenderer,
                gui->vulkan,
                text));
            link->textHash = newHash;
        }
    }

    link->url = url;

    avdRenderableTextGetSize(&link->header.text, &link->header.size.x, &link->header.size.y);

    AVD_Vector2 pos     = avdVec2Zero();
    AVD_Vector2 maxSize = avdVec2Zero();
    avdGuiResolveComponentPosition(layout, link->header.size, style, &pos, &maxSize);
    avdGuiApplyItemAlignment(layout, link->header.size, &pos, &maxSize);

    link->header.pos             = pos;
    link->header.clipRectPos     = layout->header.clipRectPos;
    link->header.clipRectSize    = layout->header.clipRectSize;
    link->header.foregroundColor = style->buttonHoverColor;
    link->header.isHovered       = avdVec2IntersectRect(gui->inputState.mousePos, pos, link->header.size);

    bool clicked = false;
    if (link->header.isHovered && gui->inputState.mouseLeftPressedThisFrame) {
        link->header.isPressed = true;
    }

    if (gui->inputState.mouseLeftReleasedThisFrame) {
        if (link->header.isPressed && link->header.isHovered) {
            clicked = true;
        }
        link->header.isPressed = false;
    }

    link->header.renderRects = __avdGuiHyperlinkRenderRects;
    link->header.renderText  = __avdGuiHyperlinkRenderText;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = linkComp;

    if (clicked) {
        avdOpenUrl(link->url);
    }

    return clicked;
}
