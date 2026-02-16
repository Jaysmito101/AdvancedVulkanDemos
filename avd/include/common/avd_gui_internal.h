#ifndef AVD_GUI_INTERNAL_H
#define AVD_GUI_INTERNAL_H

#include "common/avd_gui.h"

AVD_GuiStyle *avdGuiCurrentStyle(AVD_Gui *gui);

AVD_UInt32 avdGuiCalculateId(AVD_Gui *gui, AVD_GuiComponentType type, const char *label);

AVD_GuiComponent *avdGuiAcquireComponent(AVD_Gui *gui, AVD_GuiComponentType type, AVD_UInt32 id);

void avdGuiResolveComponentPosition(
    AVD_GuiLayoutComponent *layout,
    AVD_Vector2 itemSize,
    AVD_GuiStyle *style,
    AVD_Vector2 *outPos,
    AVD_Vector2 *outMaxSize);

void avdGuiApplyItemAlignment(
    AVD_GuiLayoutComponent *layout,
    AVD_Vector2 itemSize,
    AVD_Vector2 *pos,
    AVD_Vector2 *availableSize);

AVD_Bool avdGuiPushRect(
    AVD_Gui *gui,
    AVD_Vector2 pos,
    AVD_Vector2 size,
    AVD_Color color,
    AVD_Vector2 clipPos,
    AVD_Vector2 clipSize);

VkRect2D avdGuiScissorFromRect(AVD_Gui *gui, AVD_Vector2 pos, AVD_Vector2 size);

void avdGuiSplitLabel(
    const char *label,
    char *displayBuffer,
    size_t bufferSize,
    const char **outDisplayText,
    const char **outId);

#endif // AVD_GUI_INTERNAL_H
