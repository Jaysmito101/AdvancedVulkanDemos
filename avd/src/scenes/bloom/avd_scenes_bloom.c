#include "avd_application.h"
#include "scenes/avd_scenes.h"

static bool __avdSetupDescriptors(VkDescriptorSetLayout *layout, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(layout != NULL);

    VkDescriptorSetLayoutBinding sceneFramebufferBinding = {0};
    sceneFramebufferBinding.binding                      = 0;
    sceneFramebufferBinding.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sceneFramebufferBinding.descriptorCount              = 1;
    sceneFramebufferBinding.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo sceneFramebufferLayoutInfo = {0};
    sceneFramebufferLayoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sceneFramebufferLayoutInfo.bindingCount                    = 1;
    sceneFramebufferLayoutInfo.pBindings                       = &sceneFramebufferBinding;

    VkResult sceneLayoutResult = vkCreateDescriptorSetLayout(vulkan->device, &sceneFramebufferLayoutInfo, NULL, layout);
    AVD_CHECK_VK_RESULT(sceneLayoutResult, "Failed to create scene framebuffer descriptor set layout");
    return true;
}

static AVD_SceneBloom *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_BLOOM);
    return &scene->bloom;
}

bool avdSceneBloomCheckIntegrity(AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // Bloom scene always passes integrity check
    // as the bloom scene assets are directly embedded
    // in the application binary.
    return true;
}

bool avdSceneBloomRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneBloomCheckIntegrity;
    api->init           = avdSceneBloomInit;
    api->render         = avdSceneBloomRender;
    api->update         = avdSceneBloomUpdate;
    api->destroy        = avdSceneBloomDestroy;
    api->load           = avdSceneBloomLoad;
    api->inputEvent     = avdSceneBloomInputEvent;

    api->id = "Bloom";

    return true;
}

bool avdSceneBloomInit(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneBloom *bloom = __avdSceneGetTypePtr(scene);
    AVD_LOG("Initializing main menu scene\n");
    bloom->isBloomEnabled = true;

    // Some values that look good for the demo
    bloom->prefilterType   = AVD_BLOOM_PREFILTER_TYPE_SOFTKNEE;
    bloom->bloomThreshold  = 2.0f;
    bloom->bloomSoftKnee   = 1.2f;
    bloom->bloomAmount     = 1.0f;
    bloom->lowQuality      = false;
    bloom->applyGamma      = false; // usually we pick up a srgb framebuffer so the conversion is done by the hardware
    bloom->tonemappingType = AVD_BLOOM_TONEMAPPING_TYPE_ACES;

    AVD_CHECK(avdBloomCreate(
        &bloom->bloom,
        &appState->vulkan,
        appState->renderer.sceneFramebuffer.renderPass,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height));

    AVD_CHECK(__avdSetupDescriptors(&bloom->descriptorSetLayout, &appState->vulkan));

    AVD_CHECK(avdRenderableTextCreate(
        &bloom->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Bloom",
        512.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &bloom->uiInfoText,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "...",
        24.0f));

    return true;
}

void avdSceneBloomDestroy(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneBloom *bloom = __avdSceneGetTypePtr(scene);

    AVD_LOG("Destroying bloom scene\n");
    avdRenderableTextDestroy(&bloom->title, &appState->vulkan);
    avdRenderableTextDestroy(&bloom->uiInfoText, &appState->vulkan);
    avdBloomDestroy(&bloom->bloom, &appState->vulkan);
    vkDestroyDescriptorSetLayout(appState->vulkan.device, bloom->descriptorSetLayout, NULL);
}

bool avdSceneBloomLoad(AVD_AppState *appState, AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);
    *statusMessage = NULL;
    *progress      = 1.0f;
    return true;
}

void avdSceneBloomInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_SceneBloom *bloom = __avdSceneGetTypePtr(scene);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        } else if (event->key.key == GLFW_KEY_B && event->key.action == GLFW_PRESS) {
            bloom->isBloomEnabled = !bloom->isBloomEnabled;
        } else if (event->key.key == GLFW_KEY_P && event->key.action == GLFW_PRESS) {
            bloom->prefilterType = (bloom->prefilterType + 1) % AVD_BLOOM_PREFILTER_TYPE_COUNT;
        } else if (event->key.key == GLFW_KEY_L && event->key.action == GLFW_PRESS) {
            bloom->lowQuality = !bloom->lowQuality;
        } else if (event->key.key == GLFW_KEY_G && event->key.action == GLFW_PRESS) {
            bloom->applyGamma = !bloom->applyGamma;
        } else if (event->key.key == GLFW_KEY_M && event->key.action == GLFW_PRESS) {
            bloom->tonemappingType = (bloom->tonemappingType + 1) % AVD_BLOOM_TONEMAPPING_TYPE_COUNT;
        }
    }
}

bool avdSceneBloomUpdate(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneBloom *bloom = __avdSceneGetTypePtr(scene);

    const float scale = (float)appState->framerate.deltaTime * 2.0f;
    if (appState->input.keyState[GLFW_KEY_UP] && appState->input.keyState[GLFW_KEY_T]) {
        bloom->bloomThreshold += scale;
    } else if (appState->input.keyState[GLFW_KEY_DOWN] && appState->input.keyState[GLFW_KEY_T]) {
        bloom->bloomThreshold -= scale;
    } else if (appState->input.keyState[GLFW_KEY_UP] && appState->input.keyState[GLFW_KEY_Y]) {
        bloom->bloomSoftKnee += scale;
    } else if (appState->input.keyState[GLFW_KEY_DOWN] && appState->input.keyState[GLFW_KEY_Y]) {
        bloom->bloomSoftKnee -= scale;
    } else if (appState->input.keyState[GLFW_KEY_UP] && appState->input.keyState[GLFW_KEY_A]) {
        bloom->bloomAmount += scale;
    } else if (appState->input.keyState[GLFW_KEY_DOWN] && appState->input.keyState[GLFW_KEY_A]) {
        bloom->bloomAmount -= scale;
    }

    static char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "Bloom:\n"
             "  - Enabled: %s [B to toggle]\n"
             "  - Prefilter Type: %s [P to switch through]\n"
             "  - Threshold: %.2f [T + Up/Down to change]\n"
             "  - Soft Knee: %.2f [Y + Up/Down to change]\n"
             "  - Amount: %.2f [A + Up/Down to change]\n"
             "  - Low Quality: %s [L to toggle]\n"
             "  - Apply Gamma: %s [G to toggle]\n"
             "  - Tonemapping: %s [M to switch through]\n"
             "General Stats:\n"
             "  - Framerate: %zu FPS\n"
             "  - Frame Time: %.2f ms\n",
             bloom->isBloomEnabled ? "true" : "false",
             avdBloomPrefilterTypeToString(bloom->prefilterType),
             bloom->bloomThreshold,
             bloom->bloomSoftKnee,
             bloom->bloomAmount,
             bloom->lowQuality ? "true" : "false",
             bloom->applyGamma ? "true" : "false",
             bloom->tonemappingType == AVD_BLOOM_TONEMAPPING_TYPE_NONE ? "None" : bloom->tonemappingType == AVD_BLOOM_TONEMAPPING_TYPE_ACES ? "ACES"
                                                                              : bloom->tonemappingType == AVD_BLOOM_TONEMAPPING_TYPE_FILMIC ? "Filmic"
                                                                                                                                            : "Unknown",
             appState->framerate.fps,
             appState->framerate.deltaTime * 1000.0f);
    AVD_CHECK(avdRenderableTextUpdate(&bloom->uiInfoText,
                                      &appState->fontRenderer,
                                      &appState->vulkan,
                                      buffer));

    return true;
}

bool avdSceneBloomRender(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneBloom *bloom = __avdSceneGetTypePtr(scene);

    AVD_Vulkan *vulkan           = &appState->vulkan;
    AVD_VulkanRenderer *renderer = &appState->renderer;

    uint32_t currentFrameIndex    = renderer->currentFrameIndex;
    VkCommandBuffer commandBuffer = renderer->resources[currentFrameIndex].commandBuffer;

    float frameWidth  = (float)renderer->sceneFramebuffer.width;
    float frameHeight = (float)renderer->sceneFramebuffer.height;

    float titleWidth, titleHeight;
    float uiInfoTextWidth, uiInfoTextHeight;

    avdRenderableTextGetSize(&bloom->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&bloom->uiInfoText, &uiInfoTextWidth, &uiInfoTextHeight);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    avdRenderText(
        vulkan,
        &appState->fontRenderer,
        &bloom->title,
        commandBuffer,
        (frameWidth - 1330.0f) / 2.0f, (frameHeight + 100.0f) / 2.0f,
        0.8f, 1.2f, 4.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    avdRenderText(
        vulkan,
        &appState->fontRenderer,
        &bloom->uiInfoText,
        commandBuffer,
        10.0f, 10.0f + uiInfoTextHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    if (bloom->isBloomEnabled) {
        AVD_BloomParams params = {
            .prefilterType   = bloom->prefilterType,
            .threshold       = bloom->bloomThreshold,
            .softKnee        = bloom->bloomSoftKnee,
            .bloomAmount     = bloom->bloomAmount,
            .lowQuality      = bloom->lowQuality,
            .applyGamma      = bloom->applyGamma,
            .tonemappingType = bloom->tonemappingType};

        AVD_CHECK(avdBloomApplyInplace(
            commandBuffer,
            &bloom->bloom,
            &renderer->sceneFramebuffer,
            vulkan,
            params));
    }

    return true;
}