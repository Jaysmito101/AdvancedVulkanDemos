#include "common/avd_gui.h"
#include "common/avd_gui_internal.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "font/avd_font_renderer.h"
#include "math/avd_vector_non_simd.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_pipeline_utils.h"
#include <string.h>

static void __avdGuiClampScrollOffset(AVD_GuiLayoutComponent *layout, AVD_GuiStyle *style)
{
    if (layout->layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL) {
        AVD_Float maxScroll    = AVD_MIN(-(layout->contentSize.y - layout->header.size.y + style->padding * 2.0f), 0.0f);
        layout->scrollOffset.y = AVD_CLAMP(layout->scrollOffset.y, maxScroll, 0.0f);
    } else if (layout->layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL) {
        AVD_Float maxScroll    = AVD_MIN(-(layout->contentSize.x - layout->header.size.x + style->padding * 2.0f), 0.0f);
        layout->scrollOffset.x = AVD_CLAMP(layout->scrollOffset.x, maxScroll, 0.0f);
    }
}

static bool __avdGuiApplyWindowBounds(AVD_Gui *gui, AVD_GuiWindowComponent *window)
{
    window->header.size.x = AVD_MAX(window->header.size.x, window->minSize.x);
    window->header.size.y = AVD_MAX(window->header.size.y, window->minSize.y);

    if (window->windowType == AVD_GUI_WINDOW_TYPE_LEFT_DOCKED) {
        window->header.pos  = avdVec2(0.0f, 0.0f);
        window->header.size = avdVec2(
            AVD_CLAMP(window->header.size.x, window->minSize.x, gui->screenSize.x),
            gui->screenSize.y);
    } else if (window->windowType == AVD_GUI_WINDOW_TYPE_FULL_SCREEN) {
        window->header.pos  = avdVec2(0.0f, 0.0f);
        window->header.size = gui->screenSize;
    } else if (window->windowType == AVD_GUI_WINDOW_TYPE_RIGHT_DOCKED) {
        window->header.size = avdVec2(
            AVD_CLAMP(window->header.size.x, window->minSize.x, gui->screenSize.x),
            gui->screenSize.y);
        window->header.pos = avdVec2(gui->screenSize.x - window->header.size.x, 0.0f);
    } else if (window->windowType == AVD_GUI_WINDOW_TYPE_TOP_DOCKED) {
        window->header.pos  = avdVec2(0.0f, 0.0f);
        window->header.size = avdVec2(
            gui->screenSize.x,
            AVD_CLAMP(window->header.size.y, window->minSize.y, gui->screenSize.y));
    } else if (window->windowType == AVD_GUI_WINDOW_TYPE_BOTTOM_DOCKED) {
        window->header.size = avdVec2(
            gui->screenSize.x,
            AVD_CLAMP(window->header.size.y, window->minSize.y, gui->screenSize.y));
        window->header.pos = avdVec2(0.0f, gui->screenSize.y - window->header.size.y);
    } else {
        window->header.pos.x = AVD_CLAMP(window->header.pos.x, 0.0f, AVD_MAX(gui->screenSize.x - window->header.size.x, 0.0f));
        window->header.pos.y = AVD_CLAMP(window->header.pos.y, 0.0f, AVD_MAX(gui->screenSize.y - window->header.size.y, 0.0f));
    }

    return true;
}

static AVD_GuiInteractionMode __avdGuiDetectResizeEdge(AVD_Gui *gui)
{
    AVD_Vector2 mp   = gui->inputState.mousePos;
    AVD_Vector2 wp   = gui->window->header.pos;
    AVD_Vector2 ws   = gui->window->header.size;
    AVD_Float border = 50.0f;

    if (gui->window->windowType == AVD_GUI_WINDOW_TYPE_FLOATING) {
        // skip titile bar
        wp.y += gui->window->titleBarHeight;
        ws.y -= gui->window->titleBarHeight;
    }

    AVD_Bool nearLeft   = (mp.x >= wp.x && mp.x <= wp.x + border);
    AVD_Bool nearRight  = (mp.x >= wp.x + ws.x - border && mp.x <= wp.x + ws.x);
    AVD_Bool nearTop    = (mp.y >= wp.y && mp.y <= wp.y + border);
    AVD_Bool nearBottom = (mp.y >= wp.y + ws.y - border && mp.y <= wp.y + ws.y);

    AVD_GuiInteractionMode resizeMode = AVD_GUI_INTERACTION_NONE;

    if (nearTop && nearLeft) {
        resizeMode = AVD_GUI_INTERACTION_RESIZE_TOP_LEFT;
    } else if (nearTop && nearRight) {
        resizeMode = AVD_GUI_INTERACTION_RESIZE_TOP_RIGHT;
    } else if (nearBottom && nearLeft) {
        resizeMode = AVD_GUI_INTERACTION_RESIZE_BOTTOM_LEFT;
    } else if (nearBottom && nearRight) {
        resizeMode = AVD_GUI_INTERACTION_RESIZE_BOTTOM_RIGHT;
    } else if (nearLeft) {
        resizeMode = AVD_GUI_INTERACTION_RESIZE_LEFT;
    } else if (nearRight) {
        resizeMode = AVD_GUI_INTERACTION_RESIZE_RIGHT;
    } else if (nearTop) {
        resizeMode = AVD_GUI_INTERACTION_RESIZE_TOP;
    } else if (nearBottom) {
        resizeMode = AVD_GUI_INTERACTION_RESIZE_BOTTOM;
    }

    AVD_Bool validResize = false;
    switch (gui->window->windowType) {
        case AVD_GUI_WINDOW_TYPE_LEFT_DOCKED:
            validResize = (resizeMode == AVD_GUI_INTERACTION_RESIZE_RIGHT);
            break;
        case AVD_GUI_WINDOW_TYPE_RIGHT_DOCKED:
            validResize = (resizeMode == AVD_GUI_INTERACTION_RESIZE_LEFT);
            break;
        case AVD_GUI_WINDOW_TYPE_TOP_DOCKED:
            validResize = (resizeMode == AVD_GUI_INTERACTION_RESIZE_BOTTOM);
            break;
        case AVD_GUI_WINDOW_TYPE_BOTTOM_DOCKED:
            validResize = (resizeMode == AVD_GUI_INTERACTION_RESIZE_TOP);
            break;
        case AVD_GUI_WINDOW_TYPE_FLOATING:
            validResize = true; // all resize modes are valid for floating windows
            break;
        default:
            break;
    }
    if (!validResize) {
        resizeMode = AVD_GUI_INTERACTION_NONE;
    }

    return resizeMode;
}

static void __avdGuiApplyWindowResize(AVD_Gui *gui, AVD_Vector2 delta)
{
    AVD_GuiWindowComponent *w = gui->window;
    AVD_Vector2 pos           = w->header.pos;
    AVD_Vector2 size          = w->header.size;

    switch (w->interactionMode) {
        case AVD_GUI_INTERACTION_RESIZE_RIGHT:
            size.x += delta.x;
            break;
        case AVD_GUI_INTERACTION_RESIZE_BOTTOM:
            size.y += delta.y;
            break;
        case AVD_GUI_INTERACTION_RESIZE_LEFT:
            pos.x += delta.x;
            size.x -= delta.x;
            break;
        case AVD_GUI_INTERACTION_RESIZE_TOP:
            pos.y += delta.y;
            size.y -= delta.y;
            break;
        case AVD_GUI_INTERACTION_RESIZE_BOTTOM_RIGHT:
            size.x += delta.x;
            size.y += delta.y;
            break;
        case AVD_GUI_INTERACTION_RESIZE_BOTTOM_LEFT:
            pos.x += delta.x;
            size.x -= delta.x;
            size.y += delta.y;
            break;
        case AVD_GUI_INTERACTION_RESIZE_TOP_RIGHT:
            pos.y += delta.y;
            size.x += delta.x;
            size.y -= delta.y;
            break;
        case AVD_GUI_INTERACTION_RESIZE_TOP_LEFT:
            pos.x += delta.x;
            pos.y += delta.y;
            size.x -= delta.x;
            size.y -= delta.y;
            break;
        default:
            break;
    }

    if (size.x < w->minSize.x) {
        if (pos.x != w->header.pos.x) {
            pos.x = w->header.pos.x + w->header.size.x - w->minSize.x;
        }
        size.x = w->minSize.x;
    }
    if (size.y < w->minSize.y) {
        if (pos.y != w->header.pos.y) {
            pos.y = w->header.pos.y + w->header.size.y - w->minSize.y;
        }
        size.y = w->minSize.y;
    }

    w->header.pos  = pos;
    w->header.size = size;
    __avdGuiApplyWindowBounds(gui, w);
}

static void __avdGuiResolveChildLayoutPosSize(
    AVD_GuiLayoutComponent *parentLayout,
    AVD_GuiStyle *style,
    AVD_Vector2 *outPos,
    AVD_Vector2 *outSize)
{
    AVD_Float pw = parentLayout->header.size.x - style->padding * 2.0f;
    AVD_Float ph = parentLayout->header.size.y - style->padding * 2.0f;

    switch (parentLayout->layoutType) {
        case AVD_GUI_LAYOUT_TYPE_VERTICAL:
        case AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL: {
            *outPos = avdVec2(
                parentLayout->header.pos.x + style->padding,
                parentLayout->cursor.y + parentLayout->scrollOffset.y);
            AVD_Float remaining = 0.0f;
            if (parentLayout->layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL) {
                remaining = parentLayout->header.size.y - style->padding * 2.0f;
            } else {
                AVD_Float parentBottom = parentLayout->header.pos.y + parentLayout->header.size.y;
                remaining              = parentBottom - parentLayout->cursor.y - style->padding;
            }
            *outSize = avdVec2(pw, AVD_MAX(remaining, 0.0f));
            break;
        }
        case AVD_GUI_LAYOUT_TYPE_HORIZONTAL:
        case AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL: {
            *outPos = avdVec2(
                parentLayout->cursor.x + parentLayout->scrollOffset.x,
                parentLayout->header.pos.y + style->padding);
            AVD_Float remaining = 0.0f;
            if (parentLayout->layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL) {
                remaining = parentLayout->header.size.x - style->padding * 2.0f;
            } else {
                AVD_Float parentRight = parentLayout->header.pos.x + parentLayout->header.size.x;
                remaining             = parentRight - parentLayout->cursor.x - style->padding;
            }
            *outSize = avdVec2(AVD_MAX(remaining, 0.0f), ph);
            break;
        }
        default: {
            *outPos  = parentLayout->cursor;
            *outSize = avdVec2(pw, ph);
            break;
        }
    }
}

static void __avdGuiResolveChildLayoutClipRect(
    AVD_GuiComponent *parent,
    AVD_GuiComponentHeader *childHeader)
{
    childHeader->clipRectPos  = childHeader->pos;
    childHeader->clipRectSize = childHeader->size;

    if (parent->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
        AVD_Float cx0 = AVD_MAX(childHeader->clipRectPos.x, parent->layout.header.clipRectPos.x);
        AVD_Float cy0 = AVD_MAX(childHeader->clipRectPos.y, parent->layout.header.clipRectPos.y);
        AVD_Float cx1 = AVD_MIN(
            childHeader->clipRectPos.x + childHeader->clipRectSize.x,
            parent->layout.header.clipRectPos.x + parent->layout.header.clipRectSize.x);
        AVD_Float cy1 = AVD_MIN(
            childHeader->clipRectPos.y + childHeader->clipRectSize.y,
            parent->layout.header.clipRectPos.y + parent->layout.header.clipRectSize.y);
        childHeader->clipRectPos  = avdVec2(cx0, cy0);
        childHeader->clipRectSize = avdVec2(AVD_MAX(0.0f, cx1 - cx0), AVD_MAX(0.0f, cy1 - cy0));
    }
}

static AVD_Bool __avdGuiGetLayoutScrollbarGeometry(
    AVD_GuiLayoutComponent *layout,
    AVD_GuiComponentHeader *layoutHeader,
    AVD_GuiStyle *style,
    AVD_Bool vertical,
    AVD_Vector2 *outTrackPos,
    AVD_Vector2 *outTrackSize,
    AVD_Vector2 *outThumbPos,
    AVD_Vector2 *outThumbSize,
    AVD_Float *outRange,
    AVD_Float *outViewport,
    AVD_Float *outContent)
{
    AVD_Float viewport = vertical ? layoutHeader->size.y : layoutHeader->size.x;
    AVD_Float content  = vertical ? layout->contentSize.y : layout->contentSize.x;
    content            = AVD_MAX(content, viewport);

    if (content <= viewport + 1.0f) {
        return false;
    }

    AVD_Float thickness = AVD_MAX(6.0f, style->padding * 0.75f);
    AVD_Vector2 trackPos;
    AVD_Vector2 trackSize;

    if (vertical) {
        trackPos = avdVec2(
            layoutHeader->pos.x + layoutHeader->size.x - thickness - 2.0f,
            layoutHeader->pos.y + 2.0f);
        trackSize = avdVec2(thickness, AVD_MAX(layoutHeader->size.y - 4.0f, 1.0f));
    } else {
        trackPos = avdVec2(
            layoutHeader->pos.x + 2.0f,
            layoutHeader->pos.y + layoutHeader->size.y - thickness - 2.0f);
        trackSize = avdVec2(AVD_MAX(layoutHeader->size.x - 4.0f, 1.0f), thickness);
    }

    AVD_Float thumb  = vertical
                           ? AVD_CLAMP(trackSize.y * (viewport / content), 18.0f, trackSize.y)
                           : AVD_CLAMP(trackSize.x * (viewport / content), 18.0f, trackSize.x);
    AVD_Float range  = vertical ? AVD_MAX(trackSize.y - thumb, 0.0f) : AVD_MAX(trackSize.x - thumb, 0.0f);
    AVD_Float denom  = AVD_MAX(content - viewport, 1e-5f);
    AVD_Float offset = vertical ? layout->scrollOffset.y : layout->scrollOffset.x;
    AVD_Float t      = AVD_CLAMP((-offset) / denom, 0.0f, 1.0f);

    AVD_Vector2 thumbPos;
    AVD_Vector2 thumbSize;
    if (vertical) {
        thumbPos  = avdVec2(trackPos.x, trackPos.y + range * t);
        thumbSize = avdVec2(trackSize.x, thumb);
    } else {
        thumbPos  = avdVec2(trackPos.x + range * t, trackPos.y);
        thumbSize = avdVec2(thumb, trackSize.y);
    }

    if (outTrackPos) {
        *outTrackPos = trackPos;
    }
    if (outTrackSize) {
        *outTrackSize = trackSize;
    }
    if (outThumbPos) {
        *outThumbPos = thumbPos;
    }
    if (outThumbSize) {
        *outThumbSize = thumbSize;
    }
    if (outRange) {
        *outRange = range;
    }
    if (outViewport) {
        *outViewport = viewport;
    }
    if (outContent) {
        *outContent = content;
    }

    return true;
}

static void __avdGuiUpdateLayoutScrollbar(
    AVD_Gui *gui,
    AVD_GuiLayoutComponent *layout,
    AVD_GuiComponentHeader *layoutHeader,
    AVD_GuiStyle *style,
    AVD_Bool vertical)
{
    AVD_Bool *dragging = vertical ? &layout->verticalScrollbarDragging : &layout->horizontalScrollbarDragging;

    if (!gui->inputState.mouseLeftPressed) {
        *dragging = false;
    }

    AVD_Vector2 trackPos  = avdVec2Zero();
    AVD_Vector2 trackSize = avdVec2Zero();
    AVD_Vector2 thumbPos  = avdVec2Zero();
    AVD_Vector2 thumbSize = avdVec2Zero();
    AVD_Float range       = 0.0f;
    AVD_Float viewport    = 0.0f;
    AVD_Float content     = 0.0f;

    if (!__avdGuiGetLayoutScrollbarGeometry(
            layout,
            layoutHeader,
            style,
            vertical,
            &trackPos,
            &trackSize,
            &thumbPos,
            &thumbSize,
            &range,
            &viewport,
            &content)) {
        *dragging = false;
        return;
    }

    AVD_Bool overThumb = avdVec2IntersectRect(gui->inputState.mousePos, thumbPos, thumbSize);
    if (gui->inputState.mouseLeftPressedThisFrame && overThumb) {
        *dragging = true;
    }

    if (*dragging && range > 0.0f) {
        AVD_Float delta        = vertical ? gui->inputState.mouseDelta.y : gui->inputState.mouseDelta.x;
        AVD_Float contentDelta = (content - viewport) * (delta / range);
        if (vertical) {
            layout->scrollOffset.y -= contentDelta;
        } else {
            layout->scrollOffset.x -= contentDelta;
        }
    }
}

static AVD_Bool __avdGuiIsComponentVisible(const AVD_GuiComponentHeader *header)
{
    AVD_Float x0 = AVD_MAX(header->pos.x, header->clipRectPos.x);
    AVD_Float y0 = AVD_MAX(header->pos.y, header->clipRectPos.y);
    AVD_Float x1 = AVD_MIN(header->pos.x + header->size.x, header->clipRectPos.x + header->clipRectSize.x);
    AVD_Float y1 = AVD_MIN(header->pos.y + header->size.y, header->clipRectPos.y + header->clipRectSize.y);

    return (x1 > x0) && (y1 > y0);
}

static void __avdGuiRenderLayoutRects(AVD_Gui *gui, AVD_GuiStyle *style, AVD_GuiLayoutComponent *layout)
{
    for (AVD_Int32 i = 0; i < layout->itemCount; ++i) {
        AVD_GuiComponent *component = layout->items[i];
        if (component == NULL) {
            continue;
        }

        if (component->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
            __avdGuiRenderLayoutRects(gui, style, &component->layout);
            continue;
        }

        if (component->header.type == AVD_GUI_COMPONENT_TYPE_IMAGE || component->header.type == AVD_GUI_COMPONENT_TYPE_IMAGE_BUTTON) {
            continue;
        }

        if (!__avdGuiIsComponentVisible(&component->header)) {
            continue;
        }

        if (component->header.renderRects) {
            component->header.renderRects(gui, &component->header);
        }
    }

    AVD_Bool vertical   = layout->layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL;
    AVD_Bool horizontal = layout->layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL;
    if (vertical || horizontal) {
        AVD_Vector2 trackPos  = avdVec2Zero();
        AVD_Vector2 trackSize = avdVec2Zero();
        AVD_Vector2 thumbPos  = avdVec2Zero();
        AVD_Vector2 thumbSize = avdVec2Zero();

        if (__avdGuiGetLayoutScrollbarGeometry(
                layout,
                &layout->header,
                style,
                vertical,
                &trackPos,
                &trackSize,
                &thumbPos,
                &thumbSize,
                NULL,
                NULL,
                NULL)) {
            AVD_Bool thumbHovered = avdVec2IntersectRect(gui->inputState.mousePos, thumbPos, thumbSize);
            AVD_Color thumbColor  = thumbHovered ? style->scrollThumbHoverColor : style->scrollThumbColor;

            avdGuiPushRect(gui, trackPos, trackSize, style->scrollTrackColor, layout->header.clipRectPos, layout->header.clipRectSize);
            avdGuiPushRect(gui, thumbPos, thumbSize, thumbColor, layout->header.clipRectPos, layout->header.clipRectSize);
        }
    }
}

static void __avdGuiRenderLayoutText(AVD_Gui *gui, AVD_GuiLayoutComponent *layout, VkCommandBuffer commandBuffer)
{
    for (AVD_Int32 i = 0; i < layout->itemCount; ++i) {
        AVD_GuiComponent *component = layout->items[i];
        if (component == NULL)
            continue;

        if (component->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
            __avdGuiRenderLayoutText(gui, &component->layout, commandBuffer);
            continue;
        }

        if (!__avdGuiIsComponentVisible(&component->header)) {
            continue;
        }

        if (component->header.renderText) {
            component->header.renderText(gui, &component->header, commandBuffer);
        }
    }
}

static AVD_Bool __avdGuiPushRectInternal(
    AVD_Gui *gui,
    AVD_Vector2 pos,
    AVD_Vector2 size,
    AVD_Color color,
    AVD_Vector2 clipPos,
    AVD_Vector2 clipSize,
    AVD_Bool textured)
{
    AVD_Float x0 = AVD_MAX(pos.x, clipPos.x);
    AVD_Float y0 = AVD_MAX(pos.y, clipPos.y);
    AVD_Float x1 = AVD_MIN(pos.x + size.x, clipPos.x + clipSize.x);
    AVD_Float y1 = AVD_MIN(pos.y + size.y, clipPos.y + clipSize.y);

    AVD_Float clippedW = x1 - x0;
    AVD_Float clippedH = y1 - y0;
    if (clippedW <= 0.0f || clippedH <= 0.0f) {
        return true;
    }

    if (gui->drawVertexCount >= AVD_GUI_MAX_DRAW_VERTICES) {
        return false;
    }

    AVD_GuiStyle *style = avdGuiCurrentStyle(gui);

    AVD_Float u0 = (size.x > 0.0f) ? (x0 - pos.x) / size.x : 0.0f;
    AVD_Float v0 = (size.y > 0.0f) ? (y0 - pos.y) / size.y : 0.0f;
    AVD_Float u1 = (size.x > 0.0f) ? (x1 - pos.x) / size.x : 1.0f;
    AVD_Float v1 = (size.y > 0.0f) ? (y1 - pos.y) / size.y : 1.0f;

    AVD_Float nx = x0 / gui->screenSize.x;
    AVD_Float ny = y0 / gui->screenSize.y;
    AVD_Float nw = clippedW / gui->screenSize.x;
    AVD_Float nh = clippedH / gui->screenSize.y;

    AVD_GuiVertexData *vertex = &gui->drawVertices[gui->drawVertexCount++];
    vertex->posX              = avdQuantizeHalf(nx);
    vertex->posY              = avdQuantizeHalf(ny);
    vertex->sizeX             = avdQuantizeHalf(nw);
    vertex->sizeY             = avdQuantizeHalf(nh);
    vertex->uv0X              = avdQuantizeHalf(u0);
    vertex->uv0Y              = avdQuantizeHalf(v0);
    vertex->uv1X              = avdQuantizeHalf(u1);
    vertex->uv1Y              = avdQuantizeHalf(v1);
    vertex->color             = color;
    vertex->cornerRadius      = avdQuantizeHalf(style->cornerRadius / AVD_MAX(gui->screenSize.x, gui->screenSize.y));
    vertex->textured          = textured ? 1u : 0u;
    return true;
}

static AVD_Bool __avdGuiAddImageComponent(
    AVD_Gui *gui,
    AVD_GuiComponent *component,
    VkDescriptorSet *descriptorSets,
    AVD_UInt32 *firstInstances,
    AVD_UInt32 *drawCallCount)
{
    if (!__avdGuiIsComponentVisible(&component->header)) {
        return true;
    }

    AVD_VulkanImageSubresource *subresource = NULL;
    if (component->header.type == AVD_GUI_COMPONENT_TYPE_IMAGE) {
        subresource = component->image.subresource;
    } else if (component->header.type == AVD_GUI_COMPONENT_TYPE_IMAGE_BUTTON) {
        subresource = component->imageButton.subresource;
    }

    if (subresource == NULL) {
        return true;
    }

    VkDescriptorSet *componentDescriptorSet = NULL;
    if (component->header.type == AVD_GUI_COMPONENT_TYPE_IMAGE) {
        componentDescriptorSet = &component->image.descriptorSet;
    } else if (component->header.type == AVD_GUI_COMPONENT_TYPE_IMAGE_BUTTON) {
        componentDescriptorSet = &component->imageButton.descriptorSet;
    }

    if (componentDescriptorSet == NULL) {
        return true;
    }

    AVD_CHECK_MSG(*drawCallCount < AVD_GUI_MAX_COMPONENTS, "Exceeded maximum number of GUI draw calls");

    if (*componentDescriptorSet == VK_NULL_HANDLE) {
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        AVD_CHECK(avdAllocateDescriptorSet(
            gui->vulkan->device,
            gui->vulkan->descriptorPool,
            gui->descriptorSetLayout,
            &descriptorSet));

        VkWriteDescriptorSet writes[2]    = {0};
        VkDescriptorBufferInfo bufferInfo = gui->ssbo.descriptorBufferInfo;

        AVD_CHECK(avdWriteBufferDescriptorSet(&writes[0], descriptorSet, 0, &bufferInfo));
        AVD_CHECK(avdWriteImageDescriptorSet(&writes[1], descriptorSet, 1, &subresource->descriptorImageInfo));
        vkUpdateDescriptorSets(gui->vulkan->device, AVD_ARRAY_COUNT(writes), writes, 0, NULL);

        *componentDescriptorSet = descriptorSet;
    }

    AVD_UInt32 firstInstance = gui->drawVertexCount;
    AVD_CHECK(__avdGuiPushRectInternal(
        gui,
        component->header.pos,
        component->header.size,
        avdColorRgba(1.0f, 1.0f, 1.0f, 1.0f),
        component->header.clipRectPos,
        component->header.clipRectSize,
        true));

    descriptorSets[*drawCallCount] = *componentDescriptorSet;
    firstInstances[*drawCallCount] = firstInstance;
    *drawCallCount += 1;

    return true;
}

static AVD_Bool __avdGuiAddLayoutImages(
    AVD_Gui *gui,
    AVD_GuiLayoutComponent *layout,
    VkDescriptorSet *descriptorSets,
    AVD_UInt32 *firstInstances,
    AVD_UInt32 *drawCallCount)
{
    for (AVD_Int32 i = 0; i < layout->itemCount; ++i) {
        AVD_GuiComponent *component = layout->items[i];
        if (component == NULL) {
            continue;
        }

        if (component->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
            AVD_CHECK(__avdGuiAddLayoutImages(gui, &component->layout, descriptorSets, firstInstances, drawCallCount));
            continue;
        }

        if (component->header.type == AVD_GUI_COMPONENT_TYPE_IMAGE || component->header.type == AVD_GUI_COMPONENT_TYPE_IMAGE_BUTTON) {
            AVD_CHECK(__avdGuiAddImageComponent(gui, component, descriptorSets, firstInstances, drawCallCount));
        }
    }

    return true;
}

AVD_GuiStyle *avdGuiCurrentStyle(AVD_Gui *gui)
{
    AVD_ASSERT(gui->styleStackTop > 0);
    return &gui->styleStack[gui->styleStackTop - 1];
}

AVD_GuiComponent *avdGuiGetLastParent(AVD_Gui *gui)
{
    if (gui->parentStackTop <= 0) {
        return NULL;
    }
    return gui->parentStack[gui->parentStackTop - 1];
}

static AVD_GuiComponent *avdGuiGetLastItem(AVD_Gui *gui)
{
    if (gui->activeLayout == NULL || gui->activeLayout->header.type != AVD_GUI_COMPONENT_TYPE_LAYOUT) {
        return NULL;
    }

    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    if (layout->itemCount <= 0) {
        return NULL;
    }

    return layout->items[layout->itemCount - 1];
}

void avdGuiApplyItemAlignment(
    AVD_GuiLayoutComponent *layout,
    AVD_Vector2 itemSize,
    AVD_Vector2 *pos,
    AVD_Vector2 *availableSize)
{
    AVD_Float spaceX = AVD_MAX(availableSize->x - itemSize.x, 0.0f);
    AVD_Float spaceY = AVD_MAX(availableSize->y - itemSize.y, 0.0f);

    switch (layout->horizontalAlign) {
        case AVD_GUI_LAYOUT_ALIGN_CENTER: {
            pos->x += spaceX * 0.5f;
            break;
        }
        case AVD_GUI_LAYOUT_ALIGN_END: {
            pos->x += spaceX;
            break;
        }
        case AVD_GUI_LAYOUT_ALIGN_STRETCH:
        case AVD_GUI_LAYOUT_ALIGN_START:
        default:
            break;
    }

    switch (layout->verticalAlign) {
        case AVD_GUI_LAYOUT_ALIGN_CENTER: {
            pos->y += spaceY * 0.5f;
            break;
        }
        case AVD_GUI_LAYOUT_ALIGN_END: {
            pos->y += spaceY;
            break;
        }
        case AVD_GUI_LAYOUT_ALIGN_STRETCH:
        case AVD_GUI_LAYOUT_ALIGN_START:
        default:
            break;
    }
}

VkRect2D avdGuiScissorFromRect(AVD_Gui *gui, AVD_Vector2 pos, AVD_Vector2 size)
{
    AVD_Float minX = AVD_CLAMP(pos.x, 0.0f, gui->screenSize.x);
    AVD_Float minY = AVD_CLAMP(pos.y, 0.0f, gui->screenSize.y);
    AVD_Float maxX = AVD_CLAMP(pos.x + size.x, 0.0f, gui->screenSize.x);
    AVD_Float maxY = AVD_CLAMP(pos.y + size.y, 0.0f, gui->screenSize.y);

    VkRect2D scissor = {
        .offset = {(AVD_Int32)minX, (AVD_Int32)minY},
        .extent = {(AVD_UInt32)AVD_MAX(0.0f, maxX - minX), (AVD_UInt32)AVD_MAX(0.0f, maxY - minY)}};

    return scissor;
}

AVD_UInt32 avdGuiCalculateId(AVD_Gui *gui, AVD_GuiComponentType type, const char *label)
{
    AVD_UInt32 hash = 2166136261u;
    hash ^= (AVD_UInt32)type;
    hash *= 16777619u;
    hash ^= gui->idSeed;
    for (AVD_UInt32 i = 0; i < (AVD_UInt32)gui->idStackTop; ++i) {
        hash ^= gui->idStack[i];
        hash *= 16777619u;
    }
    hash *= 16777619u;

    if (label != NULL && label[0] != '\0') {
        const char *idStart = strstr(label, "##");
        if (idStart != NULL && idStart[2] != '\0') {
            label = idStart + 2;
        }
        while (*label != '\0') {
            hash ^= (AVD_UInt32)(*label);
            hash *= 16777619u;
            label++;
        }
    }

    if (gui->window) {
        hash ^= gui->window->header.id;
        hash *= 16777619u;
    }

    if (hash == 0) {
        hash = 1;
    }

    return hash;
}

AVD_GuiComponent *avdGuiAcquireComponent(AVD_Gui *gui, AVD_GuiComponentType type, AVD_UInt32 id)
{
    AVD_Double now = (AVD_Double)clock() / (AVD_Double)CLOCKS_PER_SEC;

    for (AVD_UInt32 i = 0; i < AVD_GUI_MAX_COMPONENTS; ++i) {
        AVD_GuiComponent *component = &gui->components[i];
        if (component->header.type == type && component->header.id == id) {
            component->header.lastUpdateTime = now;
            return component;
        }
    }

    AVD_UInt32 lruIndex   = 0;
    AVD_Double lruTime    = 1e30;
    AVD_Int32 freeSlotIdx = -1;

    for (AVD_UInt32 i = 0; i < AVD_GUI_MAX_COMPONENTS; ++i) {
        AVD_GuiComponent *component = &gui->components[i];
        if (component->header.type == AVD_GUI_COMPONENT_TYPE_NONE) {
            freeSlotIdx = (AVD_Int32)i;
            break;
        }
        if (component->header.lastUpdateTime < lruTime) {
            lruTime  = component->header.lastUpdateTime;
            lruIndex = i;
        }
    }

    AVD_GuiComponent *component = freeSlotIdx >= 0 ? &gui->components[freeSlotIdx] : &gui->components[lruIndex];
    if (component->header.text.initialized) {
        avdRenderableTextDestroy(&component->header.text, gui->vulkan);
    }

    memset(component, 0, sizeof(AVD_GuiComponent));
    component->header.type           = type;
    component->header.id             = id;
    component->header.lastUpdateTime = now;
    return component;
}

AVD_Bool avdGuiPushRect(
    AVD_Gui *gui,
    AVD_Vector2 pos,
    AVD_Vector2 size,
    AVD_Color color,
    AVD_Vector2 clipPos,
    AVD_Vector2 clipSize)
{
    return __avdGuiPushRectInternal(gui, pos, size, color, clipPos, clipSize, false);
}

static bool avdGuiCalculateChildSizeOffset(
    AVD_GuiComponent *parent,
    AVD_Vector2 *childPos,
    AVD_Vector2 *childSize,
    AVD_GuiStyle *style)
{
    AVD_ASSERT(parent != NULL);
    AVD_ASSERT(childPos != NULL);
    AVD_ASSERT(childSize != NULL);
    AVD_ASSERT(style != NULL);



    AVD_Float topOffset = style->padding;
    if (parent->header.type == AVD_GUI_COMPONENT_TYPE_WINDOW) {
        topOffset += parent->window.titleBarHeight;
    }

    if (childSize) {
        *childSize = avdVec2(
            parent->header.size.x - style->padding * 2.0f,
            parent->header.size.y - style->padding - topOffset);
    }

    if (childPos) {
        *childPos = avdVec2(
            parent->header.pos.x + style->padding,
            parent->header.pos.y + topOffset);
    }

    return true;
}

void avdGuiSplitLabel(
    const char *label,
    char *displayBuffer,
    size_t bufferSize,
    const char **outDisplayText,
    const char **outId)
{
    AVD_ASSERT(label != NULL);
    AVD_ASSERT(displayBuffer != NULL);
    AVD_ASSERT(outDisplayText != NULL);
    AVD_ASSERT(outId != NULL);

    *outDisplayText = label;
    *outId          = label;

    const char *separator = strstr(label, "##");
    if (separator != NULL) {
        size_t len = (size_t)(separator - label);
        if (len >= bufferSize) {
            len = bufferSize - 1;
        }
        memcpy(displayBuffer, label, len);
        displayBuffer[len] = '\0';
        *outDisplayText    = displayBuffer;
        *outId             = separator + 2;
    }
}

void avdGuiResolveComponentPosition(
    AVD_GuiLayoutComponent *layout,
    AVD_Vector2 itemSize,
    AVD_GuiStyle *style,
    AVD_Vector2 *outPos,
    AVD_Vector2 *outMaxSize)
{
    AVD_ASSERT(layout != NULL);

    AVD_Vector2 pos     = layout->cursor;
    AVD_Vector2 maxSize = itemSize;

    switch (layout->layoutType) {
        case AVD_GUI_LAYOUT_TYPE_VERTICAL:
        case AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL: {
            pos.x = layout->header.pos.x + style->padding;
            pos.y = layout->cursor.y + layout->scrollOffset.y;
            layout->cursor.y += itemSize.y + layout->spacing;
            maxSize.x = layout->contentSize.x - style->padding * 2.0f;
            break;
        }
        case AVD_GUI_LAYOUT_TYPE_HORIZONTAL:
        case AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL: {
            pos.x = layout->cursor.x + layout->scrollOffset.x;
            pos.y = layout->header.pos.y + style->padding;
            layout->cursor.x += itemSize.x + layout->spacing;
            maxSize.y = layout->contentSize.y - style->padding * 2.0f;
            break;
        }
        case AVD_GUI_LAYOUT_TYPE_FLOAT: {
            if (layout->cursor.x + itemSize.x + style->padding > layout->header.pos.x + layout->contentSize.x) {
                layout->cursor.x = layout->header.pos.x + style->padding;
                layout->cursor.y = layout->contentExtent.y;
            }
            pos = layout->cursor;
            layout->cursor.x += itemSize.x + layout->spacing;
            maxSize.x = layout->contentSize.x - (pos.x - layout->header.pos.x) - style->padding;
            maxSize.y = layout->contentSize.y - (pos.y - layout->header.pos.y) - style->padding;
            break;
        }
        case AVD_GUI_LAYOUT_TYPE_GRID:
        case AVD_GUI_LAYOUT_TYPE_NONE:
        case AVD_GUI_LAYOUT_TYPE_COUNT:
            AVD_ASSERT(false && "Invalid layout type");
            break;
    }

    layout->contentExtent.x = AVD_MAX(layout->contentExtent.x, pos.x + itemSize.x);
    layout->contentExtent.y = AVD_MAX(layout->contentExtent.y, pos.y + itemSize.y + layout->spacing);

    if (outPos) {
        *outPos = pos;
    }

    if (outMaxSize) {
        *outMaxSize = maxSize;
    }
}

AVD_GuiStyle avdGuiStyleDefault(void)
{
    return (AVD_GuiStyle){
        .padding                = 8.0f,
        .itemSpacing            = 4.0f,
        .cornerRadius           = 6.0f,
        .titleBarHeight         = 30.0f,
        .windowBgColor          = avdColorRgba(0.06f, 0.07f, 0.09f, 1.0f),
        .windowBgFocusedColor   = avdColorRgba(0.07f, 0.08f, 0.10f, 1.0f),
        .titleBarColor          = avdColorRgba(0.09f, 0.10f, 0.13f, 1.0f),
        .textColor              = avdColorRgba(0.82f, 0.84f, 0.88f, 1.0f),
        .buttonColor            = avdColorRgba(0.10f, 0.11f, 0.14f, 1.0f),
        .buttonHoverColor       = avdColorRgba(0.14f, 0.15f, 0.20f, 1.0f),
        .buttonPressColor       = avdColorRgba(0.08f, 0.09f, 0.12f, 1.0f),
        .buttonTextColor        = avdColorRgba(0.90f, 0.91f, 0.95f, 1.0f),
        .scrollTrackColor       = avdColorRgba(0.12f, 0.13f, 0.17f, 0.90f),
        .scrollThumbColor       = avdColorRgba(0.55f, 0.42f, 0.85f, 0.95f),
        .scrollThumbHoverColor  = avdColorRgba(0.8f, 0.8f, 0.8f, 1.0f),
        .linkColor              = avdColorRgba(0.4f, 0.6f, 1.0f, 1.0f),
        .linkHoverColor         = avdColorRgba(0.6f, 0.8f, 1.0f, 1.0f),
        .sliderTrackColor       = avdColorRgba(0.12f, 0.13f, 0.17f, 1.0f),
        .sliderThumbColor       = avdColorRgba(0.55f, 0.42f, 0.85f, 1.0f),
        .sliderThumbHoverColor  = avdColorRgba(0.65f, 0.52f, 0.95f, 1.0f),
        .sliderThumbActiveColor = avdColorRgba(0.75f, 0.62f, 1.0f, 1.0f),
        .headerColor            = avdColorRgba(0.10f, 0.11f, 0.14f, 1.0f),
        .headerHoverColor       = avdColorRgba(0.14f, 0.15f, 0.20f, 1.0f),
        .headerTextColor        = avdColorRgba(0.90f, 0.91f, 0.95f, 1.0f),
        .headerArrowColor       = avdColorRgba(0.70f, 0.72f, 0.78f, 1.0f),
        .dragBgColor            = avdColorRgba(0.12f, 0.13f, 0.17f, 1.0f),
        .dragHoverColor         = avdColorRgba(0.16f, 0.17f, 0.22f, 1.0f),
        .dragActiveColor        = avdColorRgba(0.20f, 0.21f, 0.27f, 1.0f),
        .dragTextColor          = avdColorRgba(0.82f, 0.84f, 0.88f, 1.0f),
    };
}

bool avdGuiCreate(
    AVD_Gui *gui,
    AVD_Vulkan *vulkan,
    AVD_FontManager *fontManager,
    AVD_FontRenderer *fontRenderer,
    VkRenderPass renderPass,
    AVD_Vector2 screenSize)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderPass != VK_NULL_HANDLE);

    memset(gui, 0, sizeof(AVD_Gui));

    gui->vulkan       = vulkan;
    gui->fontRenderer = fontRenderer;
    gui->screenSize   = screenSize;
    gui->idSeed       = 0x9E3779B9u;

    gui->styleStack[0] = avdGuiStyleDefault();
    gui->styleStackTop = 1;

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &gui->descriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        2,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, gui->descriptorSetLayout, "Gui/DescriptorSetLayout");

    AVD_CHECK(avdAllocateDescriptorSet(vulkan->device, vulkan->descriptorPool, gui->descriptorSetLayout, &gui->descriptorSet));
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET, gui->descriptorSet, "Gui/DescriptorSet");

    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &gui->pipelineLayout,
        &gui->pipeline,
        vulkan->device,
        &gui->descriptorSetLayout,
        1,
        sizeof(AVD_UInt32) * 4,
        renderPass,
        1,
        "GuiVert",
        "GuiFrag",
        NULL,
        NULL));
    AVD_CHECK(avdVulkanBufferCreate(
        vulkan,
        &gui->ssbo,
        sizeof(gui->drawVertices),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        "Gui/SSBO"));
    AVD_CHECK_MSG(fontManager->fontCount > 0, "Font manager has no fonts");

    VkWriteDescriptorSet writes[2]    = {0};
    VkDescriptorBufferInfo bufferInfo = gui->ssbo.descriptorBufferInfo;
    VkDescriptorImageInfo imageInfo   = fontManager->fonts[0].fontAtlasImage.defaultSubresource.descriptorImageInfo;

    AVD_CHECK(avdWriteBufferDescriptorSet(&writes[0], gui->descriptorSet, 0, &bufferInfo));
    AVD_CHECK(avdWriteImageDescriptorSet(&writes[1], gui->descriptorSet, 1, &imageInfo));
    vkUpdateDescriptorSets(vulkan->device, AVD_ARRAY_COUNT(writes), writes, 0, NULL);

    return true;
}

void avdGuiDestroy(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);

    AVD_Vulkan *vulkan = gui->vulkan;

    for (AVD_UInt32 i = 0; i < AVD_GUI_MAX_COMPONENTS; ++i) {
        if (gui->components[i].header.text.initialized) {
            avdRenderableTextDestroy(&gui->components[i].header.text, vulkan);
        }
    }

    avdVulkanBufferDestroy(vulkan, &gui->ssbo);
    vkDestroyPipeline(vulkan->device, gui->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, gui->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, gui->descriptorSetLayout, NULL);

    memset(gui, 0, sizeof(AVD_Gui));
}

void avdGuiPushStyle(AVD_Gui *gui, AVD_GuiStyle style)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->styleStackTop < (AVD_Int32)AVD_ARRAY_COUNT(gui->styleStack));
    gui->styleStack[gui->styleStackTop++] = style;
}

void avdGuiPopStyle(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->styleStackTop > 1);
    gui->styleStackTop--;
}

bool avdGuiPushEvent(AVD_Gui *gui, AVD_InputEvent *event)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(event != NULL);

    if (event->type == AVD_INPUT_EVENT_MOUSE_MOVE) {
        AVD_Vector2 mousePos = avdVec2(
            (event->mouseMove.x * 0.5f + 0.5f) * gui->screenSize.x,
            (1.0f - (event->mouseMove.y * 0.5f + 0.5f)) * gui->screenSize.y);
        gui->inputState.mouseDelta = avdVec2Subtract(mousePos, gui->inputState.mousePos);
        gui->inputState.mousePos   = mousePos;

        // window move or resize
        if (gui->interacting && gui->window != NULL) {
            if (gui->window->interactionMode == AVD_GUI_INTERACTION_DRAG) {
                gui->window->header.pos = avdVec2Subtract(gui->inputState.mousePos, gui->window->dragOffset);
                __avdGuiApplyWindowBounds(gui, gui->window);
            } else if (gui->window->interactionMode >= AVD_GUI_INTERACTION_RESIZE_LEFT && gui->window->interactionMode <= AVD_GUI_INTERACTION_RESIZE_BOTTOM_RIGHT) {
                __avdGuiApplyWindowResize(gui, gui->inputState.mouseDelta);
            }
        }
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_BUTTON) {
        if (event->mouseButton.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (event->mouseButton.action == GLFW_PRESS) {
                gui->inputState.mouseLeftPressed          = true;
                gui->inputState.mouseLeftPressedThisFrame = true;

                if (gui->window != NULL) {
                    if (gui->window->windowType == AVD_GUI_WINDOW_TYPE_FULL_SCREEN) {
                        gui->windowFocused = true;
                    } else {
                        AVD_Vector2 paddedWindowPos  = avdVec2Subtract(gui->window->header.pos, avdVec2(20.0f, 20.0f));
                        AVD_Vector2 paddedWindowSize = avdVec2Add(gui->window->header.size, avdVec2(40.0f, 40.0f));
                        AVD_Bool inWindow            = avdVec2IntersectRect(gui->inputState.mousePos, paddedWindowPos, paddedWindowSize);
                        if (inWindow) {
                            gui->windowFocused = true;

                            AVD_Bool inTitleBar = avdVec2IntersectRect(
                                gui->inputState.mousePos,
                                gui->window->header.pos,
                                avdVec2(gui->window->header.size.x, gui->window->titleBarHeight));

                            AVD_Bool onAnyChild = false;
                            for (AVD_UInt32 i = 0; i < AVD_GUI_MAX_COMPONENTS; ++i) {
                                AVD_GuiComponent *c = &gui->components[i];
                                if (c->header.type == AVD_GUI_COMPONENT_TYPE_NONE ||
                                    c->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT ||
                                    c->header.type == AVD_GUI_COMPONENT_TYPE_WINDOW)
                                    continue;
                                if (avdVec2IntersectRect(gui->inputState.mousePos, c->header.pos, c->header.size)) {
                                    onAnyChild = true;
                                    break;
                                }
                            }

                            AVD_GuiInteractionMode resizeMode = __avdGuiDetectResizeEdge(gui);
                            if (resizeMode != AVD_GUI_INTERACTION_NONE && !onAnyChild) {
                                gui->interacting             = true;
                                gui->window->interactionMode = resizeMode;
                            } else if ((gui->window->windowType == AVD_GUI_WINDOW_TYPE_FLOATING) && (inTitleBar || !onAnyChild)) {
                                gui->interacting             = true;
                                gui->window->interactionMode = AVD_GUI_INTERACTION_DRAG;
                                gui->window->dragOffset      = avdVec2Subtract(gui->inputState.mousePos, gui->window->header.pos);
                            }
                        } else {
                            gui->windowFocused = false;
                        }
                    }
                }
            } else if (event->mouseButton.action == GLFW_RELEASE) {
                gui->inputState.mouseLeftPressed           = false;
                gui->inputState.mouseLeftReleasedThisFrame = true;

                if (gui->interacting && gui->window != NULL) {
                    gui->window->interactionMode = AVD_GUI_INTERACTION_NONE;
                    gui->interacting             = false;
                }
            }
        } else if (event->mouseButton.button == GLFW_MOUSE_BUTTON_RIGHT) {
            gui->inputState.mouseRightPressed = (event->mouseButton.action == GLFW_PRESS);
        } else if (event->mouseButton.button == GLFW_MOUSE_BUTTON_MIDDLE) {
            gui->inputState.mouseMiddlePressed = (event->mouseButton.action == GLFW_PRESS);
        }

        if (gui->interacting) {
            return true;
        }
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_SCROLL) {
        gui->inputState.scrollDeltaX = event->mouseScroll.x;
        gui->inputState.scrollDeltaY = event->mouseScroll.y;
    } else if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key >= 0 && event->key.key <= GLFW_KEY_LAST) {
            gui->inputState.keysPressed[event->key.key] = (event->key.action == GLFW_PRESS || event->key.action == GLFW_REPEAT);
        }
    }

    return gui->windowFocused;
}

bool avdGuiBegin(
    AVD_Gui *gui,
    AVD_Vector2 minSize,
    AVD_GuiWindowType windowType)
{
    AVD_ASSERT(gui != NULL);

    gui->drawVertexCount = 0;

    gui->activeLayout   = NULL;
    gui->idStackTop     = 0;
    gui->parentStackTop = 0;
    gui->window         = NULL;

    AVD_GuiStyle *style = avdGuiCurrentStyle(gui);

    AVD_UInt32 resolvedId             = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_WINDOW, "AVD Gui Window");
    AVD_GuiComponent *windowComponent = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_WINDOW, resolvedId);
    AVD_GuiWindowComponent *window    = &windowComponent->window;

    if (window->header.size.x <= 0.0f || window->header.size.y <= 0.0f) {
        window->header.size = avdVec2(
            AVD_MAX(minSize.x, gui->screenSize.x * 0.5f),
            AVD_MAX(minSize.y, gui->screenSize.y * 0.5f));
        window->header.pos = avdVec2Scale(avdVec2Subtract(gui->screenSize, window->header.size), 0.5f);
    }

    if (window->windowType != windowType) {
        if (windowType == AVD_GUI_WINDOW_TYPE_LEFT_DOCKED || windowType == AVD_GUI_WINDOW_TYPE_RIGHT_DOCKED) {
            window->header.size.x = minSize.x;
        } else if (windowType == AVD_GUI_WINDOW_TYPE_TOP_DOCKED || windowType == AVD_GUI_WINDOW_TYPE_BOTTOM_DOCKED) {
            window->header.size.y = minSize.y;
        } else if (windowType == AVD_GUI_WINDOW_TYPE_FLOATING) {
            window->header.size = avdVec2(
                AVD_MAX(minSize.x, gui->screenSize.x * 0.3f),
                AVD_MAX(minSize.y, gui->screenSize.y * 0.4f));
            window->header.pos = avdVec2Scale(avdVec2Subtract(gui->screenSize, window->header.size), 0.5f);
        }
    }

    window->windowType     = windowType;
    window->minSize        = minSize;
    window->titleBarHeight = windowType == AVD_GUI_WINDOW_TYPE_FLOATING ? style->titleBarHeight : 0.0f;
    window->itemCount      = 0;

    AVD_CHECK(__avdGuiApplyWindowBounds(gui, window));

    gui->window                             = window;
    gui->parentStack[gui->parentStackTop++] = windowComponent;

    return true;
}

void avdGuiEnd(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->parentStackTop > 0);

    gui->parentStackTop                        = AVD_MAX(0, gui->parentStackTop - 1);
    gui->activeLayout                          = NULL;
    gui->inputState.mouseDelta                 = avdVec2Zero();
    gui->inputState.mouseLeftPressedThisFrame  = false;
    gui->inputState.mouseLeftReleasedThisFrame = false;
    gui->inputState.scrollDeltaX               = 0.0f;
    gui->inputState.scrollDeltaY               = 0.0f;
}

bool avdGuiBeginLayout(
    AVD_Gui *gui,
    AVD_GuiLayout layoutType,
    AVD_GuiLayoutAlign horizontalAlign,
    AVD_GuiLayoutAlign verticalAlign,
    AVD_Float spacing,
    const char *label)
{
    AVD_ASSERT(gui != NULL);

    AVD_CHECK_MSG(gui->parentStackTop > 0, "Attempting to begin layout without an active window component.");
    AVD_CHECK_MSG(gui->window != NULL, "Attempting to begin layout without an active window.");

    AVD_GuiStyle *style = avdGuiCurrentStyle(gui);

    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_LAYOUT, label);
    AVD_GuiComponent *layoutComp   = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_LAYOUT, resolvedId);
    AVD_GuiLayoutComponent *layout = &layoutComp->layout;

    layout->layoutType      = layoutType;
    layout->horizontalAlign = horizontalAlign;
    layout->verticalAlign   = verticalAlign;
    layout->spacing         = AVD_MAX(0.0f, spacing);
    layout->scrollOffset    = avdVec2Zero();

    AVD_GuiComponent *parent = avdGuiGetLastParent(gui);

    if (parent->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
        __avdGuiResolveChildLayoutPosSize(&parent->layout, style, &layoutComp->header.pos, &layoutComp->header.size);
    } else {
        AVD_CHECK(avdGuiCalculateChildSizeOffset(parent, &layoutComp->header.pos, &layoutComp->header.size, style));
    }

    __avdGuiResolveChildLayoutClipRect(parent, &layoutComp->header);
    layoutComp->layout.cursor            = avdVec2Add(layoutComp->header.pos, avdVec2(style->padding, style->padding));
    layoutComp->layout.contentExtent     = layoutComp->layout.cursor;
    layoutComp->layout.autoContentWidth  = true;
    layoutComp->layout.autoContentHeight = true;
    layoutComp->layout.contentSize       = layoutComp->header.size;
    layoutComp->layout.itemCount         = 0;

    if (parent->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
        AVD_ASSERT(parent->layout.itemCount < (AVD_Int32)AVD_ARRAY_COUNT(parent->layout.items));
        parent->layout.items[parent->layout.itemCount++] = layoutComp;
    } else {
        AVD_ASSERT(gui->window != NULL);
        AVD_ASSERT(gui->window->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(gui->window->items));
        gui->window->items[gui->window->itemCount++] = layoutComp;
    }

    gui->parentStack[gui->parentStackTop++] = layoutComp;
    gui->activeLayout                       = layoutComp;
    return true;
}

bool avdGuiBeginScrollableLayout(
    AVD_Gui *gui,
    AVD_GuiLayout layoutType,
    AVD_GuiLayoutAlign horizontalAlign,
    AVD_GuiLayoutAlign verticalAlign,
    AVD_Float spacing,
    AVD_Vector2 contentSize,
    const char *label)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL || layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL);

    AVD_CHECK_MSG(gui->parentStackTop > 0, "Attempting to begin scrollable layout without an active window component.");
    AVD_CHECK_MSG(gui->window != NULL, "Attempting to begin scrollable layout without an active window.");

    AVD_GuiStyle *style = avdGuiCurrentStyle(gui);

    AVD_UInt32 resolvedId          = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_LAYOUT, label);
    AVD_GuiComponent *layoutComp   = avdGuiAcquireComponent(gui, AVD_GUI_COMPONENT_TYPE_LAYOUT, resolvedId);
    AVD_GuiLayoutComponent *layout = &layoutComp->layout;

    layout->layoutType        = layoutType;
    layout->horizontalAlign   = horizontalAlign;
    layout->verticalAlign     = verticalAlign;
    layout->spacing           = AVD_MAX(0.0f, spacing);
    layout->autoContentWidth  = contentSize.x <= 0.0f;
    layout->autoContentHeight = contentSize.y <= 0.0f;

    if (!layout->autoContentWidth) {
        layout->contentSize.x = contentSize.x;
    } else if (layout->contentSize.x <= 0.0f) {
        layout->contentSize.x = layoutComp->header.size.x;
    }

    if (!layout->autoContentHeight) {
        layout->contentSize.y = contentSize.y;
    } else if (layout->contentSize.y <= 0.0f) {
        layout->contentSize.y = layoutComp->header.size.y;
    }

    __avdGuiClampScrollOffset(layout, style);

    AVD_GuiComponent *parent = avdGuiGetLastParent(gui);

    if (parent->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
        __avdGuiResolveChildLayoutPosSize(&parent->layout, style, &layoutComp->header.pos, &layoutComp->header.size);
    } else {
        AVD_CHECK(avdGuiCalculateChildSizeOffset(parent, &layoutComp->header.pos, &layoutComp->header.size, style));
    }

    __avdGuiResolveChildLayoutClipRect(parent, &layoutComp->header);

    if (avdVec2IntersectRect(gui->inputState.mousePos, layoutComp->header.pos, layoutComp->header.size)) {
        AVD_Float scrollSpeed = 30.0f;
        if (layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL) {
            layout->scrollOffset.y += gui->inputState.scrollDeltaY * scrollSpeed;
        } else {
            layout->scrollOffset.x += gui->inputState.scrollDeltaY * scrollSpeed;
        }
    }

    if (layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL) {
        __avdGuiUpdateLayoutScrollbar(gui, layout, &layoutComp->header, style, true);
    } else if (layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL) {
        __avdGuiUpdateLayoutScrollbar(gui, layout, &layoutComp->header, style, false);
    }

    __avdGuiClampScrollOffset(layout, style);

    layoutComp->layout.cursor        = avdVec2Add(layoutComp->header.pos, avdVec2(style->padding, style->padding));
    layoutComp->layout.contentExtent = layoutComp->layout.cursor;
    layoutComp->layout.itemCount     = 0;

    if (parent->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
        AVD_ASSERT(parent->layout.itemCount < (AVD_Int32)AVD_ARRAY_COUNT(parent->layout.items));
        parent->layout.items[parent->layout.itemCount++] = layoutComp;
    } else {
        AVD_ASSERT(gui->window != NULL);
        AVD_ASSERT(gui->window->itemCount < (AVD_Int32)AVD_ARRAY_COUNT(gui->window->items));
        gui->window->items[gui->window->itemCount++] = layoutComp;
    }

    gui->parentStack[gui->parentStackTop++] = layoutComp;
    gui->activeLayout                       = layoutComp;
    return true;
}

void avdGuiEndLayout(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);
    AVD_ASSERT(gui->parentStackTop > 0);

    AVD_GuiComponent *endedComp = gui->parentStack[gui->parentStackTop - 1];
    AVD_ASSERT(endedComp->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT);
    AVD_GuiLayoutComponent *ended = &endedComp->layout;
    AVD_GuiStyle *style           = avdGuiCurrentStyle(gui);

    gui->parentStackTop--;

    if (ended->layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL || ended->layoutType == AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL) {
        AVD_Float contentW = (ended->contentExtent.x - ended->scrollOffset.x) - ended->header.pos.x + style->padding;
        AVD_Float contentH = (ended->contentExtent.y - ended->scrollOffset.y) - ended->header.pos.y + style->padding;

        if (ended->autoContentWidth) {
            ended->contentSize.x = AVD_MAX(contentW, ended->header.size.x);
        }
        if (ended->autoContentHeight) {
            ended->contentSize.y = AVD_MAX(contentH, ended->header.size.y);
        }

        __avdGuiClampScrollOffset(ended, style);
    }

    gui->activeLayout = NULL;
    for (AVD_Int32 i = gui->parentStackTop - 1; i >= 0; --i) {
        if (gui->parentStack[i]->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
            gui->activeLayout                    = gui->parentStack[i];
            AVD_GuiLayoutComponent *parentLayout = &gui->activeLayout->layout;

            switch (parentLayout->layoutType) {
                case AVD_GUI_LAYOUT_TYPE_VERTICAL:
                case AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL:
                    parentLayout->cursor.y = AVD_MAX(
                        parentLayout->cursor.y,
                        ended->contentExtent.y - parentLayout->scrollOffset.y);
                    parentLayout->contentExtent.x = AVD_MAX(parentLayout->contentExtent.x, ended->contentExtent.x);
                    parentLayout->contentExtent.y = AVD_MAX(parentLayout->contentExtent.y, ended->contentExtent.y);
                    break;
                case AVD_GUI_LAYOUT_TYPE_HORIZONTAL:
                case AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL:
                    parentLayout->cursor.x = AVD_MAX(
                        parentLayout->cursor.x,
                        ended->contentExtent.x - parentLayout->scrollOffset.x);
                    parentLayout->cursor.y = AVD_MAX(
                        parentLayout->cursor.y,
                        ended->contentExtent.y);
                    parentLayout->contentExtent.x = AVD_MAX(parentLayout->contentExtent.x, ended->contentExtent.x);
                    parentLayout->contentExtent.y = AVD_MAX(parentLayout->contentExtent.y, ended->contentExtent.y);
                    break;
                default:
                    parentLayout->cursor.y        = AVD_MAX(parentLayout->cursor.y, ended->contentExtent.y);
                    parentLayout->contentExtent.x = AVD_MAX(parentLayout->contentExtent.x, ended->contentExtent.x);
                    parentLayout->contentExtent.y = AVD_MAX(parentLayout->contentExtent.y, ended->contentExtent.y);
                    break;
            }
            break;
        }
    }
}

AVD_Bool avdGuiIsItemHovered(AVD_Gui *gui)
{
    AVD_GuiComponent *item = avdGuiGetLastItem(gui);
    if (item == NULL)
        return false;
    return item->header.isHovered;
}

AVD_Bool avdGuiIsItemClicked(AVD_Gui *gui)
{
    AVD_GuiComponent *item = avdGuiGetLastItem(gui);
    if (item == NULL)
        return false;
    return item->header.isHovered && gui->inputState.mouseLeftReleasedThisFrame;
}

AVD_Bool avdGuiIsItemPressed(AVD_Gui *gui)
{
    AVD_GuiComponent *item = avdGuiGetLastItem(gui);
    if (item == NULL)
        return false;
    return item->header.isPressed;
}

bool avdGuiRender(AVD_Gui *gui, VkCommandBuffer commandBuffer)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    gui->drawVertexCount = 0;

    AVD_GuiStyle *style = avdGuiCurrentStyle(gui);

    if (gui->window != NULL) {
        AVD_Color bgColor = gui->windowFocused ? style->windowBgFocusedColor : style->windowBgColor;
        avdGuiPushRect(gui, gui->window->header.pos, gui->window->header.size, bgColor, gui->window->header.pos, gui->window->header.size);
        if (gui->window->titleBarHeight > 0.0f) {
            avdGuiPushRect(gui, gui->window->header.pos, avdVec2(gui->window->header.size.x, gui->window->titleBarHeight), style->titleBarColor, gui->window->header.pos, gui->window->header.size);
        }
    }

    if (gui->window != NULL) {
        for (AVD_Int32 i = 0; i < gui->window->itemCount; ++i) {
            AVD_GuiComponent *component = gui->window->items[i];
            if (component != NULL && component->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
                __avdGuiRenderLayoutRects(gui, style, &component->layout);
            }
        }
    }

    VkDescriptorSet imageDescriptorSets[AVD_GUI_MAX_COMPONENTS] = {0};
    AVD_UInt32 imageFirstInstances[AVD_GUI_MAX_COMPONENTS]      = {0};
    AVD_UInt32 imageDrawCallCount                               = 0;
    AVD_UInt32 rectDrawCallCount                                = gui->drawVertexCount;

    if (gui->window != NULL) {
        for (AVD_Int32 i = 0; i < gui->window->itemCount; ++i) {
            AVD_GuiComponent *component = gui->window->items[i];
            if (component != NULL && component->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
                AVD_CHECK(__avdGuiAddLayoutImages(
                    gui,
                    &component->layout,
                    imageDescriptorSets,
                    imageFirstInstances,
                    &imageDrawCallCount));
            }
        }
    }

    VkRect2D fullScissor = avdGuiScissorFromRect(gui, avdVec2Zero(), gui->screenSize);
    if (gui->drawVertexCount > 0) {
        AVD_CHECK(avdVulkanBufferUpload(
            gui->vulkan,
            &gui->ssbo,
            gui->drawVertices,
            gui->drawVertexCount * sizeof(AVD_GuiVertexData)));

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gui->pipeline);
        vkCmdSetScissor(commandBuffer, 0, 1, &fullScissor);

        if (rectDrawCallCount > 0) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gui->pipelineLayout, 0, 1, &gui->descriptorSet, 0, NULL);
            vkCmdDraw(commandBuffer, 6, rectDrawCallCount, 0, 0);
        }

        for (AVD_UInt32 i = 0; i < imageDrawCallCount; ++i) {
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                gui->pipelineLayout,
                0,
                1,
                &imageDescriptorSets[i],
                0,
                NULL);
            vkCmdDraw(commandBuffer, 6, 1, 0, imageFirstInstances[i]);
        }
    }

    // NOTE: [VIMP] TODO: batch the text calls!
    if (gui->window != NULL) {
        for (AVD_Int32 i = 0; i < gui->window->itemCount; ++i) {
            AVD_GuiComponent *component = gui->window->items[i];
            if (component != NULL && component->header.type == AVD_GUI_COMPONENT_TYPE_LAYOUT) {
                __avdGuiRenderLayoutText(gui, &component->layout, commandBuffer);
            }
        }
    }

    vkCmdSetScissor(commandBuffer, 0, 1, &fullScissor);

    return true;
}

bool avdGuiPushId(AVD_Gui *gui, const char *strId)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(strId != NULL);

    AVD_CHECK(gui->idStackTop < (AVD_Int32)AVD_ARRAY_COUNT(gui->idStack));
    gui->idStack[gui->idStackTop++] = avdGuiCalculateId(gui, AVD_GUI_COMPONENT_TYPE_NONE, strId);
    return true;
}

void avdGuiPopId(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->idStackTop > 0);

    gui->idStackTop = AVD_MAX(0, gui->idStackTop - 1);
}

AVD_Vector2 avdGuiGetAvailableSize(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);

    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);
    AVD_Vector2 availableSize      = avdVec2Zero();

    switch (layout->layoutType) {
        case AVD_GUI_LAYOUT_TYPE_VERTICAL:
        case AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL:
            availableSize.x = layout->contentSize.x - style->padding * 2.0f;
            availableSize.y = layout->contentSize.y - (layout->cursor.y - layout->header.pos.y) - style->padding;
            break;
        case AVD_GUI_LAYOUT_TYPE_HORIZONTAL:
        case AVD_GUI_LAYOUT_TYPE_SCROLL_HORIZONTAL:
            availableSize.x = layout->contentSize.x - (layout->cursor.x - layout->header.pos.x) - style->padding;
            availableSize.y = layout->contentSize.y - style->padding * 2.0f;
            break;
        case AVD_GUI_LAYOUT_TYPE_FLOAT:
            availableSize.x = layout->contentSize.x - (layout->cursor.x - layout->header.pos.x) - style->padding;
            availableSize.y = layout->contentSize.y - (layout->cursor.y - layout->header.pos.y) - style->padding;
            break;
        case AVD_GUI_LAYOUT_TYPE_GRID:
        case AVD_GUI_LAYOUT_TYPE_NONE:
        case AVD_GUI_LAYOUT_TYPE_COUNT:
            break;
    }

    return availableSize;
}

AVD_Vector2 avdGuiGetContentSize(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);

    return gui->activeLayout->layout.contentSize;
}

AVD_Vector2 avdGuiGetContentArea(AVD_Gui *gui)
{
    AVD_ASSERT(gui != NULL);
    AVD_ASSERT(gui->activeLayout != NULL);

    AVD_GuiLayoutComponent *layout = &gui->activeLayout->layout;
    AVD_GuiStyle *style            = avdGuiCurrentStyle(gui);

    return avdVec2(
        layout->contentSize.x - style->padding * 2.0f,
        layout->contentSize.y - style->padding * 2.0f);
}