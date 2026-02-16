#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "math/avd_vector_non_simd.h"
#include <string.h>

static void __avdGuiImageButtonRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    avdGuiPushRect(gui, header->pos, header->size, avdColorRgba(1.0f, 1.0f, 1.0f, 1.0f), header->clipRectPos, header->clipRectSize);
}

bool avdGuiImageButton(
    AVD_Gui *gui,
    const char *label,
    AVD_VulkanImageSubresource *subresource,
    AVD_Vector2 size)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);

    char displayBuffer[256];
    const char *displayText;
    const char *idStr;
    avdGuiSplitLabel(label, displayBuffer, sizeof(displayBuffer), &displayText, &idStr);

    bool clicked = false;

    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_IMAGE_BUTTON, idStr);

    AVD_Vector2 pos     = avdVec2Zero();
    AVD_Vector2 maxSize = avdVec2Zero();
    avdGuiResolveComponentPosition(layout, size, style, &pos, &maxSize);

    AVD_Vector2 resolvedSize = size;
    if (size.x == 0.0f) {
        resolvedSize.x = maxSize.x;
    }
    if (size.y == 0.0f) {
        resolvedSize.y = maxSize.y;
    }

    avdGuiApplyItemAlignment(layout, resolvedSize, &pos, &maxSize);

    AVD_GuiComponent *comp              = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_IMAGE_BUTTON, resolvedId);
    AVD_GuiImageButtonComponent *imgBtn = &comp->imageButton;

    comp->header.pos          = pos;
    comp->header.size         = resolvedSize;
    comp->header.clipRectPos  = layout->header.clipRectPos;
    comp->header.clipRectSize = layout->header.clipRectSize;
    imgBtn->subresource       = subresource;

    comp->header.isHovered = avdVec2IntersectRect(gui->inputState.mousePos, comp->header.pos, comp->header.size);

    if (gui->inputState.mouseLeftPressedThisFrame && comp->header.isHovered) {
        comp->header.isPressed = true;
    }

    if (gui->inputState.mouseLeftReleasedThisFrame) {
        if (comp->header.isPressed && comp->header.isHovered) {
            clicked = true;
        }
        comp->header.isPressed = false;
    }

    comp->header.renderRects = __avdGuiImageButtonRenderRects;
    comp->header.renderText  = NULL;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = comp;
    return clicked;
}
