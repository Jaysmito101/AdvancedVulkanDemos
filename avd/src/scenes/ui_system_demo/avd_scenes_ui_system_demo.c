#include "scenes/ui_system_demo/avd_scenes_ui_system_demo.h"
#include "avd_application.h"
#include "common/avd_gui.h"
#include "scenes/avd_scenes.h"
#include "vulkan/vulkan_core.h"

static AVD_SceneUiSystemDemo *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_UI_SYSTEM_DEMO);
    return &scene->uiSystemDemo;
}

bool avdSceneUiSystemDemoCheckIntegrity(AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;
    return true;
}

bool avdSceneUiSystemDemoRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneUiSystemDemoCheckIntegrity;
    api->init           = avdSceneUiSystemDemoInit;
    api->render         = avdSceneUiSystemDemoRender;
    api->update         = avdSceneUiSystemDemoUpdate;
    api->destroy        = avdSceneUiSystemDemoDestroy;
    api->load           = avdSceneUiSystemDemoLoad;
    api->inputEvent     = avdSceneUiSystemDemoInputEvent;

    api->id          = "Bloom";
    api->displayName = "UI System Demo";

    return true;
}

bool avdSceneUiSystemDemoInit(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneUiSystemDemo *demo = __avdSceneGetTypePtr(scene);
    AVD_LOG_INFO("Initializing UI System Demo scene");

    AVD_CHECK(avdGuiCreate(
        &demo->gui,
        &appState->vulkan,
        &appState->fontManager,
        &appState->fontRenderer,
        appState->renderer.sceneFramebuffer.renderPass,
        avdVec2((AVD_Float)appState->renderer.sceneFramebuffer.width,
                (AVD_Float)appState->renderer.sceneFramebuffer.height)));

    demo->currentWindowType = AVD_GUI_WINDOW_TYPE_FLOATING;
    demo->currentHAlign     = AVD_GUI_LAYOUT_ALIGN_START;
    demo->currentVAlign     = AVD_GUI_LAYOUT_ALIGN_START;

    demo->demoSliderValue   = 0.5f;
    demo->demoCheckboxValue = false;
    demo->demoCounter       = 0;

    return true;
}

void avdSceneUiSystemDemoDestroy(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneUiSystemDemo *demo = __avdSceneGetTypePtr(scene);

    AVD_LOG_INFO("Destroying UI System Demo scene");
    avdGuiDestroy(&demo->gui);
}

bool avdSceneUiSystemDemoLoad(AVD_AppState *appState, AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);
    *statusMessage = NULL;
    *progress      = 1.0f;
    return true;
}

void avdSceneUiSystemDemoInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_SceneUiSystemDemo *demo = __avdSceneGetTypePtr(scene);

    if (avdGuiPushEvent(&demo->gui, event)) {
        return;
    }

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        }
    }
}

bool avdSceneUiSystemDemoUpdate(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneUiSystemDemo *demo = __avdSceneGetTypePtr(scene);

    AVD_CHECK(avdGuiBegin(
        &demo->gui,
        avdVec2(400.0f, 600.0f),
        demo->currentWindowType));

    AVD_CHECK(avdGuiBeginScrollableLayout(
        &demo->gui,
        AVD_GUI_LAYOUT_TYPE_SCROLL_VERTICAL,
        demo->currentHAlign,
        demo->currentVAlign,
        6.0f,
        avdVec2(0.0f, 0.0f),
        "DemoMainLayout"));

    avdGuiLabel(&demo->gui, "UI System Demo", "RobotoCondensedRegular", 24.0f, "TitleLabel");
    avdGuiDummy(&demo->gui, avdVec2(360.0f, 2.0f), avdColorRgba(0.5f, 0.5f, 0.5f, 1.0f), "SepTitle");

    avdGuiLabel(&demo->gui, "Basic Controls", "RobotoCondensedRegular", 18.0f, "BasicHeader");

    if (avdGuiButton(&demo->gui, "Click Me!##Btn1", "RobotoCondensedRegular", 16.0f, avdVec2(360.0f, 32.0f))) {
        demo->demoCounter++;
    }

    {
        static char counterBuf[64];
        snprintf(counterBuf, sizeof(counterBuf), "Counter: %d", demo->demoCounter);
        avdGuiLabel(&demo->gui, counterBuf, "RobotoCondensedRegular", 16.0f, "CounterLabel");
    }

    avdGuiCheckbox(&demo->gui, "Enable Awesome Mode", &demo->demoCheckboxValue, "RobotoCondensedRegular", 16.0f);

    if (demo->demoCheckboxValue) {
        avdGuiLabel(&demo->gui, "Awesome mode is ENABLED!", "RobotoCondensedRegular", 14.0f, "AwesomeLabel");
    }

    avdGuiSlider(&demo->gui, "Intensity##Slider", &demo->demoSliderValue, 0.0f, 1.0f, avdVec2(360.0f, 24.0f));

    avdGuiDummy(&demo->gui, avdVec2(360.0f, 2.0f), avdColorRgba(0.5f, 0.5f, 0.5f, 1.0f), "SepLayout");

    avdGuiLabel(&demo->gui, "Layout Options", "RobotoCondensedRegular", 18.0f, "LayoutHeader");

    static const char *windowTypeNames[] = {"Floating", "FullScreen", "LeftDock", "RightDock", "TopDock", "BottomDock"};
    static const char *alignNames[]      = {"Start", "Center", "End", "Stretch"};

    {
        static char winBuf[64];
        snprintf(winBuf, sizeof(winBuf), "Window: %s##WindowTypeBtn", windowTypeNames[demo->currentWindowType]);
        if (avdGuiButton(&demo->gui, winBuf, "RobotoCondensedRegular", 14.0f, avdVec2(360.0f, 28.0f)))
            demo->currentWindowType = (demo->currentWindowType + 1) % AVD_GUI_WINDOW_TYPE_COUNT;
    }

    avdGuiEndLayout(&demo->gui);
    avdGuiEnd(&demo->gui);

    return true;
}

bool avdSceneUiSystemDemoRender(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneUiSystemDemo *demo   = __avdSceneGetTypePtr(scene);
    VkCommandBuffer commandBuffer = avdVulkanRendererGetCurrentCmdBuffer(&appState->renderer);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));
    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][Scene]:UiSystemDemo/Render");

    AVD_CHECK(avdGuiRender(&demo->gui, commandBuffer));

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);
    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}
