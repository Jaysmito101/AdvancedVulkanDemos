#include "scenes/subsurface_scattering/avd_scenes_subsurface_scattering.h"
#include "avd_application.h"
#include "scenes/avd_scenes.h"

typedef struct {
    uint32_t placeholder;
} AVD_SubSurfaceScatteringCompositePushConstants;

static AVD_SceneSubsurfaceScattering *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_SUBSURFACE_SCATTERING);
    return &scene->subsurfaceScattering;
}

static void __avdSetupBindlessDescriptorWrite(
    AVD_Vulkan *vulkan,
    AVD_VulkanDescriptorType descriptorType,
    uint32_t index,
    AVD_VulkanImage *image,
    VkWriteDescriptorSet *write)
{
    AVD_ASSERT(write != NULL);
    AVD_ASSERT(image != NULL);

    write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write->dstSet          = vulkan->bindlessDescriptorSet;
    write->descriptorCount = 1;
    write->dstBinding      = (uint32_t)descriptorType;
    write->descriptorType  = avdVulkanToVkDescriptorType(descriptorType);
    write->dstArrayElement = index;
    write->pImageInfo      = &image->descriptorImageInfo;
}

#define AVD_SETUP_BINDLESS_DEPTH_DESCRIPTOR_WRITE(index, framebuffer) \
    __avdSetupBindlessDescriptorWrite( \
        vulkan, \
        AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, \
        index, \
        &subsurfaceScattering->framebuffer.depthStencilAttachment.image, \
        &descriptorSetWrites[descriptorWriteCount++]); 

#define AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(index, framebuffer, imageIndex) \
    __avdSetupBindlessDescriptorWrite( \
        vulkan, \
        AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, \
        index, \
        &avdVulkanFramebufferGetColorAttachment(&subsurfaceScattering->framebuffer, 0)->image, \
        &descriptorSetWrites[descriptorWriteCount++]); 

static bool __avdSetupBindlessDescriptors(AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(vulkan != NULL);

    VkWriteDescriptorSet descriptorSetWrites[64] = {0};
    uint32_t descriptorWriteCount                = 0;
    AVD_SETUP_BINDLESS_DEPTH_DESCRIPTOR_WRITE(1, depthBuffer);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(2, aoBuffer, 0);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(3, lightingBuffer, 0)
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(4, lightingBuffer, 1);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(5, lightingBuffer, 2);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(6, diffusedIrradianceBuffer, 0);

    vkUpdateDescriptorSets(vulkan->device, descriptorWriteCount, descriptorSetWrites, 0, NULL);

    return true;
}

bool avdSceneSubsurfaceScatteringInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    subsurfaceScattering->loadStage    = 0;
    subsurfaceScattering->bloomEnabled = false;
    subsurfaceScattering->sceneWidth   = (uint32_t)((float)GAME_WIDTH * 1.5f);
    subsurfaceScattering->sceneHeight  = (uint32_t)((float)GAME_HEIGHT * 1.5f);

    avd3DSceneCreate(&subsurfaceScattering->models);

    AVD_CHECK(avdVulkanFramebufferCreate(
        &appState->vulkan,
        &subsurfaceScattering->depthBuffer,
        subsurfaceScattering->sceneWidth,
        subsurfaceScattering->sceneHeight,
        true,
        NULL,
        0,
        VK_FORMAT_D32_SFLOAT));

    AVD_CHECK(avdVulkanFramebufferCreate(
        &appState->vulkan,
        &subsurfaceScattering->aoBuffer,
        subsurfaceScattering->sceneWidth,
        subsurfaceScattering->sceneHeight,
        false,
        (VkFormat[]){VK_FORMAT_R16_SFLOAT},
        1, // Number of color attachments
        VK_FORMAT_D32_SFLOAT));

    AVD_CHECK(avdVulkanFramebufferCreate(
        &appState->vulkan,
        &subsurfaceScattering->lightingBuffer,
        subsurfaceScattering->sceneWidth,
        subsurfaceScattering->sceneHeight,
        false,
        (VkFormat[]){
            VK_FORMAT_R16G16B16A16_SFLOAT, // The diffuse irradiance lighting
            VK_FORMAT_R16G16B16A16_SFLOAT, // The specular irradiance lighting
            VK_FORMAT_R16G16B16A16_SFLOAT, // Extra
        },
        3, // Number of color attachments
        VK_FORMAT_D32_SFLOAT));

    AVD_CHECK(avdVulkanFramebufferCreate(
        &appState->vulkan,
        &subsurfaceScattering->diffusedIrradianceBuffer,
        subsurfaceScattering->sceneWidth,
        subsurfaceScattering->sceneHeight,
        false,
        (VkFormat[]){VK_FORMAT_R16G16B16A16_SFLOAT},
        1, // Number of color attachments
        VK_FORMAT_D32_SFLOAT));

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &subsurfaceScattering->set0Layout,
        appState->vulkan.device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));

    AVD_CHECK(avdPipelineUtilsCreateGraphicsPipelineLayout(
        &subsurfaceScattering->compositePipelineLayout,
        appState->vulkan.device,
        (VkDescriptorSetLayout[]){
            subsurfaceScattering->set0Layout,
            appState->vulkan.bindlessDescriptorSetLayout},
        2,
        sizeof(AVD_SubSurfaceScatteringCompositePushConstants)));
    AVD_CHECK(avdPipelineUtilsCreateGenericGraphicsPipeline(
        &subsurfaceScattering->compositePipeline,
        subsurfaceScattering->compositePipelineLayout,
        appState->vulkan.device,
        appState->renderer.sceneFramebuffer.renderPass,
        "FullScreenQuadVert",
        "SubSurfaceScatteringCompositeFrag"));

    AVD_CHECK(avdRenderableTextCreate(
        &subsurfaceScattering->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Subsurface Scattering  Demo",
        48.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &subsurfaceScattering->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "...",
        24.0f));

    AVD_CHECK(__avdSetupBindlessDescriptors(subsurfaceScattering, &appState->vulkan));

    return true;
}

void avdSceneSubsurfaceScatteringDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);
    avd3DSceneDestroy(&subsurfaceScattering->models);

    avdRenderableTextDestroy(&subsurfaceScattering->title, &appState->vulkan);
    avdRenderableTextDestroy(&subsurfaceScattering->info, &appState->vulkan);

    avdVulkanFramebufferDestroy(&appState->vulkan, &subsurfaceScattering->depthBuffer);
    avdVulkanFramebufferDestroy(&appState->vulkan, &subsurfaceScattering->aoBuffer);
    avdVulkanFramebufferDestroy(&appState->vulkan, &subsurfaceScattering->lightingBuffer);
    avdVulkanFramebufferDestroy(&appState->vulkan, &subsurfaceScattering->diffusedIrradianceBuffer);

    vkDestroyDescriptorSetLayout(appState->vulkan.device, subsurfaceScattering->set0Layout, NULL);
    vkDestroyPipelineLayout(appState->vulkan.device, subsurfaceScattering->compositePipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, subsurfaceScattering->compositePipeline, NULL);
}

bool avdSceneSubsurfaceScatteringLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    switch (subsurfaceScattering->loadStage) {
        case 0:
            *statusMessage = "Loading Alien Model";
            AVD_CHECK(avd3DSceneLoadObj(
                "assets/scene_subsurface_scattering/alien.obj",
                &subsurfaceScattering->models,
                AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS));
            break;
        case 1:
            *statusMessage = "Loading Buddha Model";
            AVD_CHECK(avd3DSceneLoadObj(
                "assets/scene_subsurface_scattering/buddha.obj",
                &subsurfaceScattering->models,
                AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS));
            break;
        case 2:
            *statusMessage = "Loading Standford Dragon Model";
            AVD_CHECK(avd3DSceneLoadObj(
                "assets/scene_subsurface_scattering/standford_dragon.obj",
                &subsurfaceScattering->models,
                AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS));
            break;
        case 3:
            // NOTE: Pretty dumb thing to do! Ideally we should generate the sphere
            // geometry on the fly but for now we will just load a sphere model
            // as I am out of time.
            // TODO: Revisit this and generate the sphere geometry on the fly.
            *statusMessage = "Loading Sphere Model";
            AVD_CHECK(avd3DSceneLoadObj(
                "assets/scene_subsurface_scattering/sphere.obj",
                &subsurfaceScattering->models,
                AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS));
            break;
        case 4:
            *statusMessage = "Loading Alien Thickness Map";
            break;
        case 5:
            *statusMessage = "Loading Buddha Thickness Map";
            break;
        case 6:
            *statusMessage = "Loading Standford Dragon Thickness Map";
            break;
        case 7:
            *statusMessage                  = NULL;
            *progress                       = 1.0f;
            subsurfaceScattering->loadStage = 0; // Reset load stage for next load
            AVD_LOG("Subsurface Scattering scene loaded successfully.\n");
            avd3DSceneDebugLog(&subsurfaceScattering->models, "SubsurfaceScattering/Models");
            return true;
        default:
            AVD_LOG("Subsurface Scattering scene load stage is invalid: %d\n", subsurfaceScattering->loadStage);
            return false;
    }

    subsurfaceScattering->loadStage++;
    *progress += (float)subsurfaceScattering->loadStage / 7.0f; // Update progress based on load stage
    return false;                                               // Continue loading
}

void avdSceneSubsurfaceScatteringInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        } else if (event->key.key == GLFW_KEY_B && event->key.action == GLFW_PRESS) {
            subsurfaceScattering->bloomEnabled = !subsurfaceScattering->bloomEnabled;
        }
    }
}

bool avdSceneSubsurfaceScatteringCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // Not really a dependency but we should ensure that its present
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/alien.obj");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/alien_thickness_map.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/buddha.obj");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/buddha_thickness_map.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/standford_dragon.obj");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/standford_dragon_thickness_map.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/sphere.obj");
    return true;
}

bool avdSceneSubsurfaceScatteringRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneSubsurfaceScatteringCheckIntegrity;
    api->init           = avdSceneSubsurfaceScatteringInit;
    api->render         = avdSceneSubsurfaceScatteringRender;
    api->update         = avdSceneSubsurfaceScatteringUpdate;
    api->destroy        = avdSceneSubsurfaceScatteringDestroy;
    api->load           = avdSceneSubsurfaceScatteringLoad;
    api->inputEvent     = avdSceneSubsurfaceScatteringInputEvent;

    api->displayName = "Subsurface Scattering";
    api->id          = "DDGIPlaceholder";

    return true;
}

bool avdSceneSubsurfaceScatteringUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    static char infoText[256];
    snprintf(infoText, sizeof(infoText),
             "Subsurface Scattering Demo:\n"
             "  - Bloom Enabled: %s [Press B to toggle]\n"
             "  - Press ESC to return to the main menu"
             "General Stats:\n"
             "  - Frame Rate: %zu FPS\n"
             "  - Frame Time: %.2f ms\n",
             subsurfaceScattering->bloomEnabled ? "Yes" : "No",
             appState->framerate.fps,
             appState->framerate.deltaTime * 1000.0f);

    AVD_CHECK(avdRenderableTextUpdate(&subsurfaceScattering->info,
                                      &appState->fontRenderer,
                                      &appState->vulkan,
                                      infoText));

    return true;
}

bool avdSceneSubsurfaceScatteringRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_Vulkan *vulkan           = &appState->vulkan;
    AVD_VulkanRenderer *renderer = &appState->renderer;

    uint32_t currentFrameIndex    = renderer->currentFrameIndex;
    VkCommandBuffer commandBuffer = renderer->resources[currentFrameIndex].commandBuffer;

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    AVD_CHECK(avdBeginRenderPassWithFramebuffer(
        commandBuffer,
        &subsurfaceScattering->depthBuffer,
        (VkClearValue[]){
            {.depthStencil = {.depth = 1.0f, .stencil = 0}},
        },
        1));

    AVD_CHECK(avdEndRenderPass(commandBuffer));

    // Composite to the main framebuffer
    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->compositePipeline);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&subsurfaceScattering->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&subsurfaceScattering->info, &infoWidth, &infoHeight);

    avdRenderText(
        vulkan,
        &appState->fontRenderer,
        &subsurfaceScattering->title,
        commandBuffer,
        ((float)renderer->sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);
    avdRenderText(
        vulkan,
        &appState->fontRenderer,
        &subsurfaceScattering->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    if (subsurfaceScattering->bloomEnabled) {
        AVD_BloomParams params = {
            .prefilterType   = AVD_BLOOM_PREFILTER_TYPE_SOFTKNEE,
            .threshold       = 2.0f,
            .softKnee        = 1.2f,
            .bloomAmount     = 1.0f,
            .lowQuality      = false,
            .applyGamma      = false,
            .tonemappingType = AVD_BLOOM_TONEMAPPING_TYPE_NONE};

        AVD_CHECK(avdBloomApplyInplace(
            commandBuffer,
            &appState->bloom,
            &renderer->sceneFramebuffer,
            vulkan,
            params));
    }

    return true;
}
