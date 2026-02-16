#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"
#include <string.h>

static void __avdGuiCollapsingHeaderRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    AVD_GuiCollapsingHeaderComponent *ch = (AVD_GuiCollapsingHeaderComponent *)header;
    AVD_GuiStyle *style                  = avdGuiCurrentStyle(gui);

    avdGuiPushRect(gui, header->pos, header->size, header->backgroundColor, header->clipRectPos, header->clipRectSize);

    AVD_Float arrowSize = header->size.y * 0.3f;
    AVD_Float arrowX    = header->pos.x + style->padding;
    AVD_Float arrowY    = header->pos.y + (header->size.y - arrowSize) * 0.5f;

    if (ch->open) {
        avdGuiPushRect(gui, avdVec2(arrowX, arrowY), avdVec2(arrowSize, arrowSize * 0.5f), style->headerArrowColor, header->clipRectPos, header->clipRectSize);
    } else {
        avdGuiPushRect(gui, avdVec2(arrowX, arrowY), avdVec2(arrowSize * 0.5f, arrowSize), style->headerArrowColor, header->clipRectPos, header->clipRectSize);
    }
}

static void __avdGuiCollapsingHeaderRenderText(AVD_Gui *gui, AVD_GuiComponentHeader *header, VkCommandBuffer commandBuffer)
{
    AVD_GuiCollapsingHeaderComponent *ch = (AVD_GuiCollapsingHeaderComponent *)header;
    if (!ch->header.text.initialized) {
        return;
    }

    AVD_GuiStyle *style = avdGuiCurrentStyle(gui);

    AVD_Color fc = ch->header.foregroundColor;
    AVD_Float cr = ((fc >> 0) & 0xFF) / 255.0f;
    AVD_Float cg = ((fc >> 8) & 0xFF) / 255.0f;
    AVD_Float cb = ((fc >> 16) & 0xFF) / 255.0f;
    AVD_Float ca = ((fc >> 24) & 0xFF) / 255.0f;

    AVD_Float textW = 0.0f, textH = 0.0f;
    avdRenderableTextGetSize(&ch->header.text, &textW, &textH);

    AVD_Float arrowSize = ch->header.size.y * 0.3f;
    AVD_Float textX     = ch->header.pos.x + style->padding + arrowSize + style->itemSpacing;
    AVD_Float textY     = ch->header.pos.y + (ch->header.size.y + textH) * 0.5f;

    VkRect2D scissor = avdGuiScissorFromRect(gui, ch->header.clipRectPos, ch->header.clipRectSize);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    avdRenderText(
        gui->vulkan,
        gui->fontRenderer,
        &ch->header.text,
        commandBuffer,
        textX, textY,
        1.0f,
        cr, cg, cb, ca,
        (uint32_t)gui->screenSize.x,
        (uint32_t)gui->screenSize.y);
}

bool avdGuiBeginCollapsingHeader(
    AVD_Gui *gui,
    const char *label,
    const char *fontName,
    AVD_Float charHeight,
    AVD_Float headerHeight)
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

    AVD_GuiStyle *style                  = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout       = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId                = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_COLLAPSING_HEADER, idStr);
    AVD_GuiComponent *comp               = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_COLLAPSING_HEADER, resolvedId);
    AVD_GuiCollapsingHeaderComponent *ch = &comp->collapsingHeader;

    if (ch->charHeight <= 0.0f) {
        ch->charHeight = charHeight > 0.0f ? charHeight : 20.0f;
    }

    if (!ch->header.text.initialized) {
        AVD_CHECK(avdRenderableTextCreate(
            &ch->header.text,
            gui->fontRenderer,
            gui->vulkan,
            fontName,
            displayText,
            ch->charHeight));
        ch->textHash = avdHashString(displayText);
    } else {
        AVD_UInt32 newHash = avdHashString(displayText);
        if (ch->textHash != newHash) {
            AVD_CHECK(avdRenderableTextUpdate(
                &ch->header.text,
                gui->fontRenderer,
                gui->vulkan,
                displayText));
            ch->textHash = newHash;
        }
    }

    AVD_Float resolvedHeight = headerHeight > 0.0f ? headerHeight : (ch->charHeight + style->padding * 2.0f);
    AVD_Vector2 barSize      = avdVec2(0.0f, resolvedHeight);

    AVD_Vector2 pos     = avdVec2Zero();
    AVD_Vector2 maxSize = avdVec2Zero();
    avdGuiResolveComponentPosition(layout, barSize, style, &pos, &maxSize);

    barSize.x = maxSize.x;
    avdGuiApplyItemAlignment(layout, barSize, &pos, &maxSize);

    ch->header.pos             = pos;
    ch->header.size            = barSize;
    ch->header.clipRectPos     = layout->header.clipRectPos;
    ch->header.clipRectSize    = layout->header.clipRectSize;
    ch->header.foregroundColor = style->headerTextColor;
    ch->header.isHovered       = avdVec2IntersectRect(gui->inputState.mousePos, ch->header.pos, ch->header.size);

    if (gui->inputState.mouseLeftReleasedThisFrame && ch->header.isHovered && ch->header.isPressed) {
        ch->open = !ch->open;
    }

    if (gui->inputState.mouseLeftPressedThisFrame && ch->header.isHovered) {
        ch->header.isPressed = true;
    }

    if (gui->inputState.mouseLeftReleasedThisFrame) {
        ch->header.isPressed = false;
    }

    if (ch->header.isHovered) {
        ch->header.backgroundColor = style->headerHoverColor;
    } else {
        ch->header.backgroundColor = style->headerColor;
    }

    ch->header.renderRects = __avdGuiCollapsingHeaderRenderRects;
    ch->header.renderText  = __avdGuiCollapsingHeaderRenderText;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = comp;

    if (ch->open) {
        avdGuiBeginLayout(
            gui,
            AVD_GUI_LAYOUT_TYPE_VERTICAL,
            AVD_GUI_LAYOUT_ALIGN_STRETCH,
            AVD_GUI_LAYOUT_ALIGN_START,
            style->itemSpacing,
            label);
    }

    return ch->open;
}

void avdGuiEndCollapsingHeader(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->parentStackTop > 0);

    AVD_GuiComponent *top = gui->parentStack[gui->parentStackTop - 1];
    if (top->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
        avdGuiEndLayout(gui);
    }
}
