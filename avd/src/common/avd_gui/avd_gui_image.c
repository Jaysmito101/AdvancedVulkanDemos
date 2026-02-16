#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "math/avd_vector_non_simd.h"

static void __avdGuiImageRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    avdGuiPushRect(gui, header->pos, header->size, avdColorRgba(1.0f, 1.0f, 1.0f, 1.0f), header->clipRectPos, header->clipRectSize);
}

void avdGuiImage(
    AVD_Gui *gui,
    AVD_VulkanImageSubresource *subresource,
    AVD_Vector2 size,
    const char *label)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);

    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_IMAGE, label);

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

    AVD_GuiComponent *imageComp = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_IMAGE, resolvedId);

    imageComp->header.pos          = pos;
    imageComp->header.size         = size;
    imageComp->header.clipRectPos  = layout->header.clipRectPos;
    imageComp->header.clipRectSize = layout->header.clipRectSize;
    imageComp->image.subresource   = subresource;
    imageComp->header.isHovered    = avdVec2IntersectRect(gui->inputState.mousePos, pos, size);
    imageComp->header.isPressed    = imageComp->header.isHovered && gui->inputState.mouseLeftPressed;
    imageComp->header.renderRects  = __avdGuiImageRenderRects;
    imageComp->header.renderText   = NULL;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = imageComp;
}
