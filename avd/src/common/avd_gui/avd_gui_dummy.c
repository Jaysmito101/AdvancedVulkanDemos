#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "math/avd_vector_non_simd.h"

static void __avdGuiDummyRenderRects(AVD_Gui *gui, AVD_GuiComponentHeader *header)
{
    avdGuiPushRect(gui, header->pos, header->size, header->backgroundColor, header->clipRectPos, header->clipRectSize);
}

void avdGuiDummy(
    AVD_Gui *gui,
    AVD_Vector2 size,
    AVD_Color color,
    const char *label)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);
    AVD_ASSERT(gui->activeLayout->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT);

    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);
    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_DUMMY, label);

    AVD_Vector2 pos     = avdVec2(0.0f, 0.0f);
    AVD_Vector2 maxSize = avdVec2(0.0f, 0.0f);
    avdGuiResolveComponentPosition(layout, size, style, &pos, &maxSize);
    avdGuiApplyItemAlignment(layout, size, &pos, &maxSize);

    AVD_GuiComponent *dummy = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_DUMMY, resolvedId);

    dummy->header.pos             = pos;
    dummy->header.size            = size;
    dummy->header.clipRectPos     = layout->header.clipRectPos;
    dummy->header.clipRectSize    = layout->header.clipRectSize;
    dummy->header.backgroundColor = color;
    dummy->header.isHovered       = avdVec2IntersectRect(gui->inputState.mousePos, pos, size);
    dummy->header.isPressed       = dummy->header.isHovered && gui->inputState.mouseLeftPressed;
    dummy->header.renderRects     = __avdGuiDummyRenderRects;
    dummy->header.renderText      = NULL;

    AVD_ASSERT(layout->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(layout->items));
    layout->items[layout->itemCount++] = dummy;
}
