#include "scenes/subsurface_scattering/avd_scenes_subsurface_scattering.h"
#include "avd_application.h"
#include "scenes/avd_scenes.h"

typedef struct {
    AVD_Matrix4x4 modelMatrix;
    AVD_Matrix4x4 viewMatrix;
    AVD_Matrix4x4 projectionMatrix;

    int32_t vertexOffset;
    int32_t vertexCount;
    int32_t pad0;
    int32_t pad1;

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
    AVD_SETUP_BINDLESS_DEPTH_DESCRIPTOR_WRITE(11, lightingBuffer);

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
        true,
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

    AVD_VulkanPipelineCreationInfo pipelineCreationInfo = {0};
    avdPipelineUtilsPipelineCreationInfoInit(&pipelineCreationInfo);
    pipelineCreationInfo.enableDepthTest = true;
    pipelineCreationInfo.enableBlend     = true;
    // pipelineCreationInfo.cullMode        = VK_CULL_MODE_BACK_BIT;
    pipelineCreationInfo.cullMode  = VK_CULL_MODE_NONE;
    pipelineCreationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
        "SubSurfaceScatteringSceneVert",
        "SubSurfaceScatteringLightingFrag",
        &pipelineCreationInfo));

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
        "SubSurfaceScatteringCompositeFrag",
        NULL));

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

    subsurfaceScattering->currentFocusModelIndex = 0;
    subsurfaceScattering->cameraPosition         = avdVec3(2.0f, 2.0f, 2.0f);
    subsurfaceScattering->cameraTarget           = avdVec3(0.0f, 0.0f, 0.0f); // Will be updated in Update based on index

    subsurfaceScattering->alienPosition           = avdVec3(-6.0f, 0.0f, 0.0f);
    subsurfaceScattering->buddhaPosition          = avdVec3(6.0f, 0.0f, 0.0f);
    subsurfaceScattering->standfordDragonPosition = avdVec3(0.0f, 0.0f, 0.0f);

    subsurfaceScattering->isDragging = false;
    subsurfaceScattering->lastMouseX = 0.0f;
    subsurfaceScattering->lastMouseY = 0.0f;

    AVD_Vector3 diff                   = avdVec3Subtract(subsurfaceScattering->cameraPosition, subsurfaceScattering->cameraTarget);
    subsurfaceScattering->cameraRadius = avdVec3Length(diff);
    if (subsurfaceScattering->cameraRadius < 0.001f) {                                                // Avoid division by zero if position and target are same
        subsurfaceScattering->cameraRadius = 15.0f;                                                   // Default radius
        diff                               = avdVec3(0.0f, 0.0f, subsurfaceScattering->cameraRadius); // Point along Z
    }
    subsurfaceScattering->cameraPhi   = acosf(diff.y / subsurfaceScattering->cameraRadius);
    subsurfaceScattering->cameraTheta = atan2f(diff.z, diff.x);

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

bool avdSceneSubsurfaceScatteringCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

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
            vkUpdateDescriptorSets(appState->vulkan.device, 1, &descriptorSetWrite, 0, NULL);
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
    const float mouseDragSensitivity                    = 2.0f;
    const float mouseScrollSensitivity                  = 1.0f;
    const float minRadius                               = 2.0f;
    const float maxRadius                               = 50.0f;
    const float minPhi                                  = 0.1f;
    const float maxPhi                                  = AVD_PI - 0.1f;

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        } else if (event->key.key == GLFW_KEY_B && event->key.action == GLFW_PRESS) {
            subsurfaceScattering->bloomEnabled = !subsurfaceScattering->bloomEnabled;
        } else if (event->key.key == GLFW_KEY_F && event->key.action == GLFW_PRESS) {
            subsurfaceScattering->currentFocusModelIndex = (subsurfaceScattering->currentFocusModelIndex + 1) % 3;
        }
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_BUTTON) {
        if (event->mouseButton.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (event->mouseButton.action == GLFW_PRESS) {
                subsurfaceScattering->isDragging = true;
                subsurfaceScattering->lastMouseX = appState->input.mouseX;
                subsurfaceScattering->lastMouseY = appState->input.mouseY;
            } else if (event->mouseButton.action == GLFW_RELEASE) {
                subsurfaceScattering->isDragging = false;
            }
        }
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_MOVE) {
        if (subsurfaceScattering->isDragging) {
            float deltaX = appState->input.mouseX - subsurfaceScattering->lastMouseX;
            float deltaY = appState->input.mouseY - subsurfaceScattering->lastMouseY;

            subsurfaceScattering->cameraTheta += deltaX * mouseDragSensitivity;
            subsurfaceScattering->cameraPhi -= deltaY * mouseDragSensitivity;

            subsurfaceScattering->cameraPhi = avdClamp(subsurfaceScattering->cameraPhi, minPhi, maxPhi);

            subsurfaceScattering->lastMouseX = appState->input.mouseX;
            subsurfaceScattering->lastMouseY = appState->input.mouseY;
        }
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_SCROLL) {
        subsurfaceScattering->cameraRadius -= event->mouseScroll.y * mouseScrollSensitivity;
        subsurfaceScattering->cameraRadius = avdClamp(subsurfaceScattering->cameraRadius, minRadius, maxRadius);
    }
}

bool avdSceneSubsurfaceScatteringUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    switch (subsurfaceScattering->currentFocusModelIndex) {
        case 0: // Standford Dragon
            subsurfaceScattering->cameraTarget = subsurfaceScattering->standfordDragonPosition;
            break;
        case 1: // Alien
            subsurfaceScattering->cameraTarget = subsurfaceScattering->alienPosition;
            break;
        case 2: // Buddha
            subsurfaceScattering->cameraTarget = subsurfaceScattering->buddhaPosition;
            break;
    }

    subsurfaceScattering->cameraPosition.x = subsurfaceScattering->cameraTarget.x + subsurfaceScattering->cameraRadius * sinf(subsurfaceScattering->cameraPhi) * cosf(subsurfaceScattering->cameraTheta);
    subsurfaceScattering->cameraPosition.y = subsurfaceScattering->cameraTarget.y + subsurfaceScattering->cameraRadius * cosf(subsurfaceScattering->cameraPhi);
    subsurfaceScattering->cameraPosition.z = subsurfaceScattering->cameraTarget.z + subsurfaceScattering->cameraRadius * sinf(subsurfaceScattering->cameraPhi) * sinf(subsurfaceScattering->cameraTheta);

    static char infoText[512];
    const char *currentFocusName = "Unknown";
    switch (subsurfaceScattering->currentFocusModelIndex) {
        case 0:
            currentFocusName = "Standford Dragon";
            break;
        case 1:
            currentFocusName = "Alien";
            break;
        case 2:
            currentFocusName = "Buddha";
            break;
    }

    snprintf(infoText, sizeof(infoText),
             "Subsurface Scattering Demo:\n"
             "  - Bloom Enabled: %s [Press B to toggle]\n"
             "  - Focus Target: %s [Press F to cycle]\n"
             "  - Camera: Drag LMB to orbit, Scroll to zoom\n"
             "  - Press ESC to return to the main menu\n"
             "General Stats:\n"
             "  - Frame Rate: %zu FPS\n"
             "  - Frame Time: %.2f ms\n",
             subsurfaceScattering->bloomEnabled ? "Yes" : "No",
             currentFocusName,
             appState->framerate.fps,
             appState->framerate.deltaTime * 1000.0f);

    AVD_CHECK(avdRenderableTextUpdate(&subsurfaceScattering->info,
                                      &appState->fontRenderer,
                                      &appState->vulkan,
                                      infoText));

    subsurfaceScattering->viewMatrix = avdMatLookAt(
        subsurfaceScattering->cameraPosition,
        subsurfaceScattering->cameraTarget,
        avdVec3(0.0f, 1.0f, 0.0f));

    subsurfaceScattering->projectionMatrix = avdMatPerspective(
        avdDeg2Rad(67.0f),
        (float)subsurfaceScattering->sceneWidth / (float)subsurfaceScattering->sceneHeight,
        0.1f,
        1000.0f);

    return true;
}

static bool __avdSceneRenderFirstMesh(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, uint32_t modelIndex, VkPipelineLayout pipelineLayout, AVD_Matrix4x4 modelMatrix)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    if (modelIndex >= subsurfaceScattering->models.modelsList.count) {
        AVD_LOG("Invalid model index: %u\n", modelIndex);
        return false;
    }

    AVD_Model *model = (AVD_Model *)avdListGet(&subsurfaceScattering->models.modelsList, modelIndex);
    AVD_Mesh *mesh   = (AVD_Mesh *)avdListGet(&model->meshes, 0); // We only render the first mesh for now

    // NOTE: We arent using the indices for now as the models loaded from obj files
    // are garunteed to have a single index for a single vertex and no re-use,
    // however for a proper renderer we need to use the indices.

    AVD_SubSurfaceScatteringUberPushConstants pushConstants = {
        .modelMatrix      = modelMatrix,
        .viewMatrix       = subsurfaceScattering->viewMatrix,
        .projectionMatrix = subsurfaceScattering->projectionMatrix,
        .vertexOffset     = mesh->indexOffset,
        .vertexCount      = mesh->triangleCount * 3,
    };
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdDraw(commandBuffer, mesh->triangleCount * 3, 1, 0, 0);

    return true;
}

static bool __avdSceneRenderAlien(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, VkPipelineLayout pipelineLayout)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(pipelineLayout != VK_NULL_HANDLE);

    AVD_Matrix4x4 modelMatrix = avdMatScale(0.5f, 0.5f, 0.5f);
    modelMatrix               = avdMat4x4Multiply(
        avdMatTranslation(
            subsurfaceScattering->alienPosition.x,
            subsurfaceScattering->alienPosition.y,
            subsurfaceScattering->alienPosition.z),
        modelMatrix);

    return __avdSceneRenderFirstMesh(commandBuffer, subsurfaceScattering, 0, pipelineLayout, modelMatrix);
}

static bool __avdSceneRenderBuddha(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, VkPipelineLayout pipelineLayout)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(pipelineLayout != VK_NULL_HANDLE);

    AVD_Matrix4x4 modelMatrix = avdMatScale(0.5f, 0.5f, 0.5f);
    modelMatrix               = avdMat4x4Multiply(
        avdMatTranslation(
            subsurfaceScattering->buddhaPosition.x,
            subsurfaceScattering->buddhaPosition.y,
            subsurfaceScattering->buddhaPosition.z),
        modelMatrix);

    return __avdSceneRenderFirstMesh(commandBuffer, subsurfaceScattering, 1, pipelineLayout, modelMatrix);
}

static bool __avdSceneRenderStandfordDragon(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, VkPipelineLayout pipelineLayout)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(pipelineLayout != VK_NULL_HANDLE);

    AVD_Matrix4x4 modelMatrix = avdMatScale(0.5f, 0.5f, 0.5f);
    modelMatrix               = avdMat4x4Multiply(
        avdMatTranslation(
            subsurfaceScattering->standfordDragonPosition.x,
            subsurfaceScattering->standfordDragonPosition.y,
            subsurfaceScattering->standfordDragonPosition.z),
        modelMatrix);

    return __avdSceneRenderFirstMesh(commandBuffer, subsurfaceScattering, 2, pipelineLayout, modelMatrix);
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
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
            {.depthStencil = {1.0f, 0}},
        },
        4));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->lightingPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->lightingPipelineLayout, 0, 1, &subsurfaceScattering->set0, 0, NULL);

    __avdSceneRenderStandfordDragon(commandBuffer, subsurfaceScattering, subsurfaceScattering->lightingPipelineLayout);
    __avdSceneRenderAlien(commandBuffer, subsurfaceScattering, subsurfaceScattering->lightingPipelineLayout);
    __avdSceneRenderBuddha(commandBuffer, subsurfaceScattering, subsurfaceScattering->lightingPipelineLayout);

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
