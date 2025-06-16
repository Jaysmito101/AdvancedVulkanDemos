#include "scenes/subsurface_scattering/avd_scenes_subsurface_scattering.h"
#include "avd_application.h"
#include "scenes/avd_scenes.h"

typedef struct {
    AVD_Matrix4x4 modelMatrix;
    AVD_Matrix4x4 viewMatrix;
    AVD_Matrix4x4 projectionMatrix;
} AVD_SubSurfaceScatteringUberPushConstants;

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

#define AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(index, image) \
    __avdSetupBindlessDescriptorWrite(                          \
        vulkan,                                                 \
        AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,      \
        index,                                                  \
        &subsurfaceScattering->image,                           \
        &descriptorSetWrites[descriptorWriteCount++]);

#define AVD_SETUP_BINDLESS_DEPTH_DESCRIPTOR_WRITE(index, framebuffer)    \
    __avdSetupBindlessDescriptorWrite(                                   \
        vulkan,                                                          \
        AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,               \
        index,                                                           \
        &subsurfaceScattering->framebuffer.depthStencilAttachment.image, \
        &descriptorSetWrites[descriptorWriteCount++]);

#define AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(index, framebuffer, imageIndex)                    \
    __avdSetupBindlessDescriptorWrite(                                                         \
        vulkan,                                                                                \
        AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,                                     \
        index,                                                                                 \
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
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(8, alienThicknessMap);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(9, buddhaThicknessMap);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(10, standfordDragonThicknessMap);

    vkUpdateDescriptorSets(vulkan->device, descriptorWriteCount, descriptorSetWrites, 0, NULL);

    return true;
}

bool __avdSceneCreateFramebuffers(AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(appState != NULL);

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

    return true;
}

bool __avdSceneCreatePipelines(AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(appState != NULL);

    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &subsurfaceScattering->lightingPipelineLayout,
        &subsurfaceScattering->lightingPipeline,
        appState->vulkan.device,
        (VkDescriptorSetLayout[]){
            subsurfaceScattering->set0Layout},
        1,
        sizeof(AVD_SubSurfaceScatteringUberPushConstants),
        subsurfaceScattering->lightingBuffer.renderPass,
        (uint32_t)subsurfaceScattering->lightingBuffer.colorAttachments.count,
        // "SubSurfaceScatteringSceneVert",
        "FullScreenQuadVert",
        "SubSurfaceScatteringLightingFrag"));

    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &subsurfaceScattering->compositePipelineLayout,
        &subsurfaceScattering->compositePipeline,
        appState->vulkan.device,
        (VkDescriptorSetLayout[]){
            appState->vulkan.bindlessDescriptorSetLayout},
        1,
        sizeof(AVD_SubSurfaceScatteringUberPushConstants),
        appState->renderer.sceneFramebuffer.renderPass,
        (uint32_t)appState->renderer.sceneFramebuffer.colorAttachments.count,
        "FullScreenQuadVert",
        "SubSurfaceScatteringCompositeFrag"));

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

    AVD_CHECK(__avdSceneCreateFramebuffers(subsurfaceScattering, appState));

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &subsurfaceScattering->set0Layout,
        appState->vulkan.device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT));

    AVD_CHECK(__avdSceneCreatePipelines(subsurfaceScattering, appState));

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

    avdVulkanBufferDestroy(&appState->vulkan, &subsurfaceScattering->vertexBuffer);

    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->alienThicknessMap);
    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->buddhaThicknessMap);
    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->standfordDragonThicknessMap);

    vkDestroyDescriptorSetLayout(appState->vulkan.device, subsurfaceScattering->set0Layout, NULL);

    vkDestroyPipelineLayout(appState->vulkan.device, subsurfaceScattering->compositePipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, subsurfaceScattering->compositePipeline, NULL);

    vkDestroyPipelineLayout(appState->vulkan.device, subsurfaceScattering->lightingPipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, subsurfaceScattering->lightingPipeline, NULL);
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
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/alien_thickness_map.png",
                &subsurfaceScattering->alienThicknessMap));
            break;
        case 5:
            *statusMessage = "Loading Buddha Thickness Map";
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/buddha_thickness_map.png",
                &subsurfaceScattering->buddhaThicknessMap));
            break;
        case 6:
            *statusMessage = "Loading Standford Dragon Thickness Map";
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/standford_dragon_thickness_map.png",
                &subsurfaceScattering->standfordDragonThicknessMap));
            break;
        case 7:
            *statusMessage    = "Setting Up GPU buffers";
            size_t bufferSize = subsurfaceScattering->models.modelResources.verticesList.count * subsurfaceScattering->models.modelResources.verticesList.itemSize;
            AVD_CHECK(avdVulkanBufferCreate(
                &appState->vulkan,
                &subsurfaceScattering->vertexBuffer,
                bufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
            AVD_CHECK(avdVulkanBufferUpload(
                &appState->vulkan,
                &subsurfaceScattering->vertexBuffer,
                subsurfaceScattering->models.modelResources.verticesList.items,
                bufferSize));

            VkDescriptorSetAllocateInfo allocateInfo = {0};
            allocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocateInfo.descriptorPool              = appState->vulkan.descriptorPool;
            allocateInfo.descriptorSetCount          = 1;
            allocateInfo.pSetLayouts                 = &subsurfaceScattering->set0Layout;
            AVD_CHECK_VK_RESULT(vkAllocateDescriptorSets(
                                    appState->vulkan.device,
                                    &allocateInfo,
                                    &subsurfaceScattering->set0),
                                "Failed to allocate descriptor set");
            VkWriteDescriptorSet descriptorSetWrite = {0};
            AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite,
                                                  subsurfaceScattering->set0,
                                                  0,
                                                  &subsurfaceScattering->vertexBuffer.descriptorBufferInfo));
            break;
        case 8:
            *statusMessage = "Setting Up Bindless Descriptors";
            AVD_CHECK(__avdSetupBindlessDescriptors(subsurfaceScattering, &appState->vulkan));
            break;
        case 9:
            AVD_LOG("Subsurface Scattering scene loaded successfully.\n");
            avd3DSceneDebugLog(&subsurfaceScattering->models, "SubsurfaceScattering/Models");
            break;
        default:
            AVD_LOG("Subsurface Scattering scene load stage is invalid: %d\n", subsurfaceScattering->loadStage);
            return false;
    }

    subsurfaceScattering->loadStage++;
    *progress = (float)subsurfaceScattering->loadStage / 9.0f;
    return *progress > 1.0f;
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

bool __avdSceneRenderBloomIfNeeded(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

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
            &appState->renderer.sceneFramebuffer,
            &appState->vulkan,
            params));
    }

    return true;
}

bool __avdSceneRenderDepthPass(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    return true;
}

bool __avdSceneRenderAOPass(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    return true;
}

bool __avdSceneRenderLightingPass(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    AVD_CHECK(avdBeginRenderPassWithFramebuffer(
        commandBuffer,
        &subsurfaceScattering->lightingBuffer,
        (VkClearValue[]){
            {
                .color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}, // Diffuse irradiance
            },
            {
                .color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}, // Specular irradiance
            },
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}} // Extra
        },
        3));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->lightingPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->lightingPipelineLayout, 0, 1, &subsurfaceScattering->set0, 0, NULL);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    AVD_CHECK(avdEndRenderPass(commandBuffer));

    return true;
}

bool __avdSceneRenderIrradianceDiffusionPass(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    return true;
}

bool __avdSceneRenderCompositePass(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->compositePipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->compositePipelineLayout, 0, 1, &appState->vulkan.bindlessDescriptorSet, 0, NULL);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&subsurfaceScattering->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&subsurfaceScattering->info, &infoWidth, &infoHeight);

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &subsurfaceScattering->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &subsurfaceScattering->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}

bool avdSceneSubsurfaceScatteringRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    VkCommandBuffer commandBuffer                       = appState->renderer.resources[appState->renderer.currentFrameIndex].commandBuffer;
    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    AVD_CHECK(__avdSceneRenderDepthPass(commandBuffer, subsurfaceScattering, appState));
    AVD_CHECK(__avdSceneRenderAOPass(commandBuffer, subsurfaceScattering, appState));
    AVD_CHECK(__avdSceneRenderLightingPass(commandBuffer, subsurfaceScattering, appState));
    AVD_CHECK(__avdSceneRenderIrradianceDiffusionPass(commandBuffer, subsurfaceScattering, appState));
    AVD_CHECK(__avdSceneRenderCompositePass(commandBuffer, subsurfaceScattering, appState));
    AVD_CHECK(__avdSceneRenderBloomIfNeeded(commandBuffer, subsurfaceScattering, appState));

    return true;
}
