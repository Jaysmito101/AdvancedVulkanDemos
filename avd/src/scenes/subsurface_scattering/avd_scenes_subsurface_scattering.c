#include "scenes/subsurface_scattering/avd_scenes_subsurface_scattering.h"
#include "avd_application.h"
#include "scenes/avd_scenes.h"

typedef struct {
    AVD_Matrix4x4 viewModelMatrix;
    AVD_Matrix4x4 projectionMatrix;

    AVD_Vector4 lightA;
    AVD_Vector4 lightB;

    AVD_Vector4 cameraPosition;
    AVD_Vector4 screenSize;

    int32_t vertexOffset;
    int32_t vertexCount;
    int32_t renderingLight;
    int32_t hasPBRTextures;

    uint32_t albedoTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t ormTextureIndex; // Occlusion, Roughness, Metallic
    uint32_t thicknessTextureIndex;
} AVD_SubSurfaceScatteringUberPushConstants;

#define AVD_SSS_RENDER_MODE_RESULT                             0
#define AVD_SSS_RENDER_MODE_SCENE_ALBEDO                       1
#define AVD_SSS_RENDER_MODE_SCENE_NORMAL                       2
#define AVD_SSS_RENDER_MODE_SCENE_THICKNESS_ROUGHNESS_METALLIC 3
#define AVD_SSS_RENDER_MODE_SCENE_POSITION                     4
#define AVD_SSS_RENDER_MODE_SCENE_DEPTH                        5
#define AVD_SSS_RENDER_MODE_SCENE_AO                           6
#define AVD_SSS_RENDER_MODE_SCENE_DIFFUSE                      7
#define AVD_SSS_RENDER_MODE_SCENE_SPECULAR                     8
#define AVD_SSS_RENDER_MODE_SCENE_DIFFUSED_IRRADIANCE          9
#define AVD_SSS_ALIEN_THICKNESS_MAP                            10
#define AVD_SSS_BUDDHA_THICKNESS_MAP                           11
#define AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP                  12
#define AVD_SSS_BUDDHA_ORM_MAP                                 13
#define AVD_SSS_BUDDHA_ALBEDO_MAP                              14
#define AVD_SSS_BUDDHA_NORMAL_MAP                              15
#define AVD_SSS_NOISE_TEXTURE                                  16
#define AVD_SSS_RENDER_MODE_COUNT                              17

typedef struct {
    int32_t renderMode;
} AVD_SubSurfaceScatteringCompositePushConstants;

static AVD_SceneSubsurfaceScattering *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_SUBSURFACE_SCATTERING);
    return &scene->subsurfaceScattering;
}

static const char *__avdSceneSubsurfaceScatteringGetRenderModeName(int32_t renderMode)
{
    switch (renderMode) {
        case AVD_SSS_RENDER_MODE_RESULT:
            return "Result";
        case AVD_SSS_RENDER_MODE_SCENE_ALBEDO:
            return "Scene Albedo";
        case AVD_SSS_RENDER_MODE_SCENE_NORMAL:
            return "Scene Normal";
        case AVD_SSS_RENDER_MODE_SCENE_THICKNESS_ROUGHNESS_METALLIC:
            return "Scene Thickness, Roughness, Metallic";
        case AVD_SSS_RENDER_MODE_SCENE_DEPTH:
            return "Scene Depth";
        case AVD_SSS_RENDER_MODE_SCENE_POSITION:
            return "Scene Position";
        case AVD_SSS_RENDER_MODE_SCENE_AO:
            return "Scene AO";
        case AVD_SSS_RENDER_MODE_SCENE_DIFFUSE:
            return "Scene Diffuse";
        case AVD_SSS_RENDER_MODE_SCENE_SPECULAR:
            return "Scene Specular";
        case AVD_SSS_RENDER_MODE_SCENE_DIFFUSED_IRRADIANCE:
            return "Scene Diffused Irradiance";
        case AVD_SSS_ALIEN_THICKNESS_MAP:
            return "Alien Thickness Map";
        case AVD_SSS_BUDDHA_THICKNESS_MAP:
            return "Buddha Thickness Map";
        case AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP:
            return "Stanford Dragon Thickness Map";
        case AVD_SSS_BUDDHA_ORM_MAP:
            return "Buddha ORM Map";
        case AVD_SSS_BUDDHA_ALBEDO_MAP:
            return "Buddha Albedo Map";
        case AVD_SSS_BUDDHA_NORMAL_MAP:
            return "Buddha Normal Map";
        case AVD_SSS_NOISE_TEXTURE:
            return "Noise Texture";
        default:
            AVD_ASSERT(false && "Unknown render mode");
            return "Unknown Render Mode";
    }
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

#define AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(index, framebuffer, imageIndex)                             \
    __avdSetupBindlessDescriptorWrite(                                                                  \
        vulkan,                                                                                         \
        AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,                                              \
        index,                                                                                          \
        &avdVulkanFramebufferGetColorAttachment(&subsurfaceScattering->framebuffer, imageIndex)->image, \
        &descriptorSetWrites[descriptorWriteCount++]);

static bool __avdSetupBindlessDescriptors(AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(vulkan != NULL);

    VkWriteDescriptorSet descriptorSetWrites[64] = {0};
    uint32_t descriptorWriteCount                = 0;

    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_ALBEDO, gBuffer, 0);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_NORMAL, gBuffer, 1);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_THICKNESS_ROUGHNESS_METALLIC, gBuffer, 2);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_POSITION, gBuffer, 3);
    AVD_SETUP_BINDLESS_DEPTH_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_DEPTH, gBuffer);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_AO, aoBuffer, 0);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_DIFFUSE, lightingBuffer, 0);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_SPECULAR, lightingBuffer, 1);
    AVD_SETUP_BINDLESS_DESCRIPTOR_WRITE(AVD_SSS_RENDER_MODE_SCENE_DIFFUSED_IRRADIANCE, diffusedIrradianceBuffer, 0);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(AVD_SSS_ALIEN_THICKNESS_MAP, alienThicknessMap);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(AVD_SSS_BUDDHA_THICKNESS_MAP, buddhaThicknessMap);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP, standfordDragonThicknessMap);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(AVD_SSS_BUDDHA_ORM_MAP, buddhaORMMap);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(AVD_SSS_BUDDHA_ALBEDO_MAP, buddhaAlbedoMap);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(AVD_SSS_BUDDHA_NORMAL_MAP, buddhaNormalMap);
    AVD_SETUP_BINDLESS_IMAGE_DESCRIPTOR_WRITE(AVD_SSS_NOISE_TEXTURE, noiseTexture);

    vkUpdateDescriptorSets(vulkan->device, descriptorWriteCount, descriptorSetWrites, 0, NULL);

    return true;
}

bool __avdSceneCreateFramebuffers(AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(appState != NULL);

    AVD_CHECK(avdVulkanFramebufferCreate(
        &appState->vulkan,
        &subsurfaceScattering->gBuffer,
        subsurfaceScattering->sceneWidth,
        subsurfaceScattering->sceneHeight,
        true,
        (VkFormat[]){
            VK_FORMAT_R8G8B8A8_UNORM,           // Albedo
            VK_FORMAT_A2R10G10B10_UNORM_PACK32, // Normal
            VK_FORMAT_R8G8B8A8_UNORM,           // Thickness, Roughness, Metallic
            VK_FORMAT_R16G16B16A16_SFLOAT,      // World Position
        },
        4,
        VK_FORMAT_D16_UNORM));

    AVD_CHECK(avdVulkanFramebufferCreate(
        &appState->vulkan,
        &subsurfaceScattering->aoBuffer,
        subsurfaceScattering->sceneWidth >> 1,
        subsurfaceScattering->sceneHeight >> 1,
        false,
        (VkFormat[]){VK_FORMAT_R8_UNORM},
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
        },
        2,
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
    pipelineCreationInfo.enableBlend     = false;
    // pipelineCreationInfo.cullMode        = VK_CULL_MODE_BACK_BIT;
    pipelineCreationInfo.cullMode  = VK_CULL_MODE_NONE;
    pipelineCreationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &subsurfaceScattering->gBufferPipelineLayout,
        &subsurfaceScattering->gBufferPipeline,
        appState->vulkan.device,
        (VkDescriptorSetLayout[]){
            subsurfaceScattering->set0Layout,
            appState->vulkan.bindlessDescriptorSetLayout,
        },
        2,
        sizeof(AVD_SubSurfaceScatteringUberPushConstants),
        subsurfaceScattering->gBuffer.renderPass,
        (uint32_t)subsurfaceScattering->gBuffer.colorAttachments.count,
        "SubSurfaceScatteringSceneVert",
        "SubSurfaceScatteringGBufferFrag",
        &pipelineCreationInfo));

    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &subsurfaceScattering->aoPipelineLayout,
        &subsurfaceScattering->aoPipeline,
        appState->vulkan.device,
        &appState->vulkan.bindlessDescriptorSetLayout,
        1,
        sizeof(AVD_SubSurfaceScatteringUberPushConstants),
        subsurfaceScattering->aoBuffer.renderPass,
        (uint32_t)subsurfaceScattering->aoBuffer.colorAttachments.count,
        "FullScreenQuadVert",
        "SubSurfaceScatteringAOFrag",
        &pipelineCreationInfo));

    // AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
    //     &subsurfaceScattering->lightingPipelineLayout,
    //     &subsurfaceScattering->lightingPipeline,
    //     appState->vulkan.device,
    //     &appState->vulkan.bindlessDescriptorSetLayout,
    //     1,
    //     sizeof(AVD_SubSurfaceScatteringUberPushConstants),
    //     subsurfaceScattering->lightingBuffer.renderPass,
    //     (uint32_t)subsurfaceScattering->lightingBuffer.colorAttachments.count,
    //     "FullScreenQuadVert",
    //     "SubSurfaceScatteringLightingFrag",
    //     &pipelineCreationInfo));

    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &subsurfaceScattering->compositePipelineLayout,
        &subsurfaceScattering->compositePipeline,
        appState->vulkan.device,
        &appState->vulkan.bindlessDescriptorSetLayout,
        1,
        sizeof(AVD_SubSurfaceScatteringCompositePushConstants),
        appState->renderer.sceneFramebuffer.renderPass,
        (uint32_t)appState->renderer.sceneFramebuffer.colorAttachments.count,
        "FullScreenQuadVert",
        "SubSurfaceScatteringCompositeFrag",
        NULL));

    return true;
}

bool __avdSceneFillModelInfos(AVD_SceneSubsurfaceScattering *subsurfaceScattering)
{
    AVD_ASSERT(subsurfaceScattering != NULL);

    // The Dragon model
    snprintf(subsurfaceScattering->modelsInfo[0].name, sizeof(subsurfaceScattering->modelsInfo[0].name), "Dragon");
    subsurfaceScattering->modelsInfo[0].modelIndex            = 0;
    subsurfaceScattering->modelsInfo[0].position              = avdVec3(-5.0f, 0.0f, 0.0f);
    subsurfaceScattering->modelsInfo[0].rotation              = avdVec3(0.0f, 0.0f, 0.0f);
    subsurfaceScattering->modelsInfo[0].scale                 = avdVec3(0.02f, 0.02f, 0.02f);
    subsurfaceScattering->modelsInfo[0].hasPBRTextures        = false;
    subsurfaceScattering->modelsInfo[0].albedoTextureIndex    = 0;
    subsurfaceScattering->modelsInfo[0].normalTextureIndex    = 0;
    subsurfaceScattering->modelsInfo[0].ormTextureIndex       = 0;
    subsurfaceScattering->modelsInfo[0].thicknessTextureIndex = AVD_SSS_ALIEN_THICKNESS_MAP;

    // The Standford Dragon model
    snprintf(subsurfaceScattering->modelsInfo[1].name, sizeof(subsurfaceScattering->modelsInfo[1].name), "Stanford Dragon");
    subsurfaceScattering->modelsInfo[1].modelIndex            = 2;
    subsurfaceScattering->modelsInfo[1].position              = avdVec3(0.0f, 0.0f, 0.0f);
    subsurfaceScattering->modelsInfo[1].rotation              = avdVec3(0.0f, 0.0f, 0.0f);
    subsurfaceScattering->modelsInfo[1].scale                 = avdVec3(0.5f, 0.5f, 0.5f);
    subsurfaceScattering->modelsInfo[1].hasPBRTextures        = false;
    subsurfaceScattering->modelsInfo[1].albedoTextureIndex    = 0;
    subsurfaceScattering->modelsInfo[1].normalTextureIndex    = 0;
    subsurfaceScattering->modelsInfo[1].ormTextureIndex       = 0;
    subsurfaceScattering->modelsInfo[1].thicknessTextureIndex = AVD_SSS_STANFORD_DRAGON_THICKNESS_MAP;

    // The Buddha model
    snprintf(subsurfaceScattering->modelsInfo[2].name, sizeof(subsurfaceScattering->modelsInfo[2].name), "Buddha");
    subsurfaceScattering->modelsInfo[2].modelIndex            = 1;
    subsurfaceScattering->modelsInfo[2].position              = avdVec3(5.0f, 0.0f, 0.0f);
    subsurfaceScattering->modelsInfo[2].rotation              = avdVec3(0.0f, -90.0f, 0.0f);
    subsurfaceScattering->modelsInfo[2].scale                 = avdVec3(0.02f, 0.02f, 0.02f);
    subsurfaceScattering->modelsInfo[2].hasPBRTextures        = true;
    subsurfaceScattering->modelsInfo[2].albedoTextureIndex    = AVD_SSS_BUDDHA_ALBEDO_MAP;
    subsurfaceScattering->modelsInfo[2].normalTextureIndex    = AVD_SSS_BUDDHA_NORMAL_MAP;
    subsurfaceScattering->modelsInfo[2].ormTextureIndex       = AVD_SSS_BUDDHA_ORM_MAP;
    subsurfaceScattering->modelsInfo[2].thicknessTextureIndex = AVD_SSS_BUDDHA_THICKNESS_MAP;

    return true;
}

bool avdSceneSubsurfaceScatteringInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    AVD_Float scalar = 1.5f;

    subsurfaceScattering->loadStage    = 0;
    subsurfaceScattering->bloomEnabled = false;
    subsurfaceScattering->sceneWidth   = (uint32_t)((float)GAME_WIDTH * scalar);
    subsurfaceScattering->sceneHeight  = (uint32_t)((float)GAME_HEIGHT * scalar);

    subsurfaceScattering->currentFocusModelIndex = 1;
    subsurfaceScattering->cameraPosition         = avdVec3(2.0f, 2.0f, 2.0f);
    subsurfaceScattering->cameraTarget           = avdVec3(0.0f, 0.0f, 0.0f); // Will be updated in Update based on index
    subsurfaceScattering->cameraRadius           = 3.0; // Will be updated in Update based on index
    subsurfaceScattering->cameraPhi              = sinf(AVD_PI / 4.0f) * 2.0f; // 45 degrees in radians
    subsurfaceScattering->cameraTheta            = cosf(AVD_PI / 4.0f) * 2.0f; // 45 degrees in radians

    subsurfaceScattering->renderMode = AVD_SSS_RENDER_MODE_RESULT;
    subsurfaceScattering->isDragging = false;
    subsurfaceScattering->lastMouseX = 0.0f;
    subsurfaceScattering->lastMouseY = 0.0f;

    AVD_CHECK(__avdSceneFillModelInfos(subsurfaceScattering));

    avd3DSceneCreate(&subsurfaceScattering->models);

    AVD_CHECK(__avdSceneCreateFramebuffers(subsurfaceScattering, appState));

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &subsurfaceScattering->set0Layout,
        appState->vulkan.device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT));

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

    avdVulkanFramebufferDestroy(&appState->vulkan, &subsurfaceScattering->gBuffer);
    avdVulkanFramebufferDestroy(&appState->vulkan, &subsurfaceScattering->aoBuffer);
    avdVulkanFramebufferDestroy(&appState->vulkan, &subsurfaceScattering->lightingBuffer);
    avdVulkanFramebufferDestroy(&appState->vulkan, &subsurfaceScattering->diffusedIrradianceBuffer);

    avdVulkanBufferDestroy(&appState->vulkan, &subsurfaceScattering->vertexBuffer);

    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->alienThicknessMap);
    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->buddhaThicknessMap);
    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->standfordDragonThicknessMap);
    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->buddhaORMMap);
    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->buddhaAlbedoMap);
    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->buddhaNormalMap);
    avdVulkanImageDestroy(&appState->vulkan, &subsurfaceScattering->noiseTexture);

    vkDestroyDescriptorSetLayout(appState->vulkan.device, subsurfaceScattering->set0Layout, NULL);

    vkDestroyPipelineLayout(appState->vulkan.device, subsurfaceScattering->gBufferPipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, subsurfaceScattering->gBufferPipeline, NULL);

    vkDestroyPipelineLayout(appState->vulkan.device, subsurfaceScattering->compositePipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, subsurfaceScattering->compositePipeline, NULL);

    vkDestroyPipelineLayout(appState->vulkan.device, subsurfaceScattering->lightingPipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, subsurfaceScattering->lightingPipeline, NULL);

    vkDestroyPipelineLayout(appState->vulkan.device, subsurfaceScattering->aoPipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, subsurfaceScattering->aoPipeline, NULL);
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
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/buddha_orm_map.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/buddha_albedo_map.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_subsurface_scattering/buddha_normal_map.png");
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
            *statusMessage = "Nothing Really Happening, this exists so that the loading bar can be shown before the heavy loading starts...";
            break;
        case 1:
            *statusMessage = "Creating Framebuffers";
            AVD_CHECK(__avdSceneCreatePipelines(subsurfaceScattering, appState));
            break;
        case 2:
            *statusMessage = "Loading Alien Model";
            AVD_CHECK(avd3DSceneLoadObj(
                "assets/scene_subsurface_scattering/alien.obj",
                &subsurfaceScattering->models,
                AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS));
            break;
        case 3:
            *statusMessage = "Loading Buddha Model";
            AVD_CHECK(avd3DSceneLoadObj(
                "assets/scene_subsurface_scattering/buddha.obj",
                &subsurfaceScattering->models,
                AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS));
            break;
        case 4:
            *statusMessage = "Loading Standford Dragon Model";
            AVD_CHECK(avd3DSceneLoadObj(
                "assets/scene_subsurface_scattering/standford_dragon.obj",
                &subsurfaceScattering->models,
                AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS));
            break;
        case 5:
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
        case 6:
            *statusMessage = "Loading Alien Thickness Map";
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/alien_thickness_map.png",
                &subsurfaceScattering->alienThicknessMap));
            break;
        case 7:
            *statusMessage = "Loading Buddha Thickness Map";
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/buddha_thickness_map.png",
                &subsurfaceScattering->buddhaThicknessMap));
            break;
        case 8:
            *statusMessage = "Loading Standford Dragon Thickness Map";
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/standford_dragon_thickness_map.png",
                &subsurfaceScattering->standfordDragonThicknessMap));
            break;
        case 9:
            *statusMessage = "Loading Buddha ORM Map";
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/buddha_orm_map.png",
                &subsurfaceScattering->buddhaORMMap));
            break;
        case 10:
            *statusMessage = "Loading Buddha Albedo Map";
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/buddha_albedo_map.png",
                &subsurfaceScattering->buddhaAlbedoMap));
            break;
        case 11:
            *statusMessage = "Loading Buddha Normal Map";
            AVD_CHECK(avdVulkanImageLoadFromFile(
                &appState->vulkan,
                "assets/scene_subsurface_scattering/buddha_normal_map.png",
                &subsurfaceScattering->buddhaNormalMap));
            break;
        case 12:
            *statusMessage = "Generating Noise Texture for AO";
            AVD_CHECK(avdVulkanImageCreate(
                &appState->vulkan,
                &subsurfaceScattering->noiseTexture,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                64,
                64));
            uint8_t noiseTextureData[64 * 64 * 4] = {0};
            for (uint32_t i = 0; i < 64 * 64; i++) {
                noiseTextureData[i * 4 + 0] = (uint8_t)(rand() % 256);
                noiseTextureData[i * 4 + 1] = (uint8_t)(rand() % 256);
                noiseTextureData[i * 4 + 2] = 0;
                noiseTextureData[i * 4 + 3] = 255; // Alpha channel
            }
            AVD_CHECK(avdVulkanImageUploadSimple(
                &appState->vulkan,
                &subsurfaceScattering->noiseTexture,
                noiseTextureData));
            break;
        case 13:
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
        case 14:
            *statusMessage = "Setting Up Bindless Descriptors";
            AVD_CHECK(__avdSetupBindlessDescriptors(subsurfaceScattering, &appState->vulkan));
            break;
        case 15:
            AVD_LOG("Subsurface Scattering scene loaded successfully.\n");
            avd3DSceneDebugLog(&subsurfaceScattering->models, "SubsurfaceScattering/Models");
            break;
        default:
            AVD_LOG("Subsurface Scattering scene load stage is invalid: %d\n", subsurfaceScattering->loadStage);
            return false;
    }

    subsurfaceScattering->loadStage++;
    *progress = (float)subsurfaceScattering->loadStage / 15.0f;
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
        } else if (event->key.key == GLFW_KEY_R && event->key.action == GLFW_PRESS) {
            subsurfaceScattering->renderMode = (subsurfaceScattering->renderMode + 1) % AVD_SSS_RENDER_MODE_COUNT;
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
            subsurfaceScattering->cameraPhi += deltaY * mouseDragSensitivity;

            subsurfaceScattering->cameraPhi = avdClamp(subsurfaceScattering->cameraPhi, minPhi, maxPhi);

            subsurfaceScattering->lastMouseX = appState->input.mouseX;
            subsurfaceScattering->lastMouseY = appState->input.mouseY;
        }
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_SCROLL) {
        subsurfaceScattering->cameraRadius -= event->mouseScroll.y * mouseScrollSensitivity;
        subsurfaceScattering->cameraRadius = avdClamp(subsurfaceScattering->cameraRadius, minRadius, maxRadius);
    }
}

bool __avdSceneUpdateCamera(AVD_SceneSubsurfaceScattering* subsurfaceScattering)
{
    subsurfaceScattering->cameraTarget     = subsurfaceScattering->modelsInfo[subsurfaceScattering->currentFocusModelIndex].position;
    subsurfaceScattering->cameraPosition.x = subsurfaceScattering->cameraTarget.x + subsurfaceScattering->cameraRadius * sinf(subsurfaceScattering->cameraPhi) * cosf(subsurfaceScattering->cameraTheta);
    subsurfaceScattering->cameraPosition.y = subsurfaceScattering->cameraTarget.y + subsurfaceScattering->cameraRadius * cosf(subsurfaceScattering->cameraPhi);
    subsurfaceScattering->cameraPosition.z = subsurfaceScattering->cameraTarget.z + subsurfaceScattering->cameraRadius * sinf(subsurfaceScattering->cameraPhi) * sinf(subsurfaceScattering->cameraTheta);

    subsurfaceScattering->viewMatrix = avdMatLookAt(
        subsurfaceScattering->cameraPosition,
        subsurfaceScattering->cameraTarget,
        avdVec3(0.0f, 1.0f, 0.0f));

    subsurfaceScattering->projectionMatrix = avdMatPerspective(
        avdDeg2Rad(67.0f),
        (float)subsurfaceScattering->sceneWidth / (float)subsurfaceScattering->sceneHeight,
        0.1f,
        100.0f);

    subsurfaceScattering->viewProjectionMatrix = avdMat4x4Multiply(
        subsurfaceScattering->projectionMatrix,
        subsurfaceScattering->viewMatrix);
}

bool __avdSceneUpdateLightingForModel(uint32_t sceneModelIndex, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_Float time)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(sceneModelIndex < 3);

    const AVD_Float lightSpeed = 1.0f;
    const AVD_Float radius     = 1.5f;

    AVD_Matrix4x4 unscaledModelMatrix = avdMatCalculateTransform(
        subsurfaceScattering->modelsInfo[sceneModelIndex].position,
        subsurfaceScattering->modelsInfo[sceneModelIndex].rotation,
        avdVec3(1.0f, 1.0f, 1.0f));
    
        
    AVD_Float seedA        = (AVD_Float)sceneModelIndex * 1.234f;
    AVD_Float seedB        = (AVD_Float)sceneModelIndex * 5.678f;
    AVD_Float phaseOffsetA = seedA * AVD_PI;
    AVD_Float phaseOffsetB = seedB * AVD_PI;

    AVD_Vector3 lightAPosition = avdVec3(
        radius * cosf(time * lightSpeed + phaseOffsetA),
        radius * sinf(time * lightSpeed + phaseOffsetA + AVD_PI / 3.0f),
        radius * sinf(time * lightSpeed + phaseOffsetA + AVD_PI / 6.0f));
    AVD_Vector3 lightBPosition = avdVec3(
        radius * cosf(time * lightSpeed + phaseOffsetB + AVD_PI),
        radius * sinf(time * lightSpeed + phaseOffsetB + AVD_PI / 2.0f),
        radius * sinf(time * lightSpeed + phaseOffsetB + AVD_PI / 4.0f));

    AVD_Vector4 lightAWorldSpacePosition = avdMat4x4MultiplyVec4(unscaledModelMatrix, avdVec4FromVec3(lightAPosition, 1.0f));
    AVD_Vector4 lightBWorldSpacePosition = avdMat4x4MultiplyVec4(unscaledModelMatrix, avdVec4FromVec3(lightBPosition, 1.0f));

    subsurfaceScattering->modelsInfo[sceneModelIndex].lightPositionA = lightAWorldSpacePosition;
    subsurfaceScattering->modelsInfo[sceneModelIndex].lightPositionB = lightBWorldSpacePosition;

    return true;
}

bool __avdSceneUpdateLighting(AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_Float time)
{
    AVD_ASSERT(subsurfaceScattering != NULL);

    // Update lighting for each model in the scene
    for (uint32_t i = 0; i < AVD_ARRAY_COUNT(subsurfaceScattering->modelsInfo); i++) {
        AVD_CHECK(__avdSceneUpdateLightingForModel(i, subsurfaceScattering, time));
    }

    return true;
}

bool avdSceneSubsurfaceScatteringUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneSubsurfaceScattering *subsurfaceScattering = __avdSceneGetTypePtr(scene);

    AVD_CHECK(__avdSceneUpdateCamera(subsurfaceScattering));
    AVD_CHECK(__avdSceneUpdateLighting(subsurfaceScattering, (AVD_Float)appState->framerate.currentTime));

    static char infoText[512];
    const char *currentFocusName = subsurfaceScattering->modelsInfo[subsurfaceScattering->currentFocusModelIndex].name;
    snprintf(infoText, sizeof(infoText),
             "Subsurface Scattering Demo:\n"
             "  - Bloom Enabled: %s [Press B to toggle]\n"
             "  - Focus Target: %s [Press F to cycle]\n"
             "  - Render Mode: %s [Press R to cycle]\n"
             "  - Camera: Drag LMB to orbit, Scroll to zoom\n"
             "  - Press ESC to return to the main menu\n"
             "General Stats:\n"
             "  - Frame Rate: %zu FPS\n"
             "  - Frame Time: %.2f ms\n",
             subsurfaceScattering->bloomEnabled ? "Yes" : "No",
             currentFocusName,
             __avdSceneSubsurfaceScatteringGetRenderModeName(subsurfaceScattering->renderMode),
             appState->framerate.fps,
             appState->framerate.deltaTime * 1000.0f);

    AVD_CHECK(avdRenderableTextUpdate(&subsurfaceScattering->info,
                                      &appState->fontRenderer,
                                      &appState->vulkan,
                                      infoText));

    return true;
}

static bool __avdSceneRenderFirstMesh(
    VkCommandBuffer commandBuffer,
    AVD_SceneSubsurfaceScattering *subsurfaceScattering,
    VkPipelineLayout pipelineLayout,
    AVD_Float time,
    uint32_t sceneModelIndex,
    bool renderLightSpheres)
{
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(pipelineLayout != VK_NULL_HANDLE);
    AVD_ASSERT(sceneModelIndex < 3);

    uint32_t modelIndex = subsurfaceScattering->modelsInfo[sceneModelIndex].modelIndex;

    AVD_Model *sphereModel = (AVD_Model *)avdListGet(&subsurfaceScattering->models.modelsList, subsurfaceScattering->models.modelsList.count - 1);
    AVD_Mesh *sphereMesh   = (AVD_Mesh *)avdListGet(&sphereModel->meshes, 0); // We only render the first mesh for now

    AVD_Model *model = (AVD_Model *)avdListGet(&subsurfaceScattering->models.modelsList, modelIndex);
    AVD_Mesh *mesh   = (AVD_Mesh *)avdListGet(&model->meshes, 0); // We only render the first mesh for now

    AVD_Matrix4x4 modelMatrix = avdMatCalculateTransform(
        subsurfaceScattering->modelsInfo[sceneModelIndex].position,
        subsurfaceScattering->modelsInfo[sceneModelIndex].rotation,
        subsurfaceScattering->modelsInfo[sceneModelIndex].scale);

    // NOTE: We arent using the indices for now as the models loaded from obj files
    // are garunteed to have a single index for a single vertex and no re-use,
    // however for a proper renderer we need to use the indices.

    AVD_SubSurfaceScatteringUberPushConstants pushConstants = {0};
    pushConstants.projectionMatrix                          = subsurfaceScattering->projectionMatrix;
    pushConstants.viewModelMatrix                           = avdMat4x4Multiply(subsurfaceScattering->viewMatrix, modelMatrix);
    pushConstants.lightA                                    = subsurfaceScattering->modelsInfo[sceneModelIndex].lightPositionA;
    pushConstants.lightB                                    = subsurfaceScattering->modelsInfo[sceneModelIndex].lightPositionB;
    pushConstants.cameraPosition                            = avdVec4FromVec3(subsurfaceScattering->cameraPosition, 1.0f);
    pushConstants.vertexOffset                              = mesh->indexOffset;
    pushConstants.vertexCount                               = mesh->triangleCount * 3;
    pushConstants.screenSize.x                              = (AVD_Float)subsurfaceScattering->sceneWidth;
    pushConstants.screenSize.y                              = (AVD_Float)subsurfaceScattering->sceneHeight;
    pushConstants.hasPBRTextures                            = subsurfaceScattering->modelsInfo[sceneModelIndex].hasPBRTextures;
    pushConstants.albedoTextureIndex                        = subsurfaceScattering->modelsInfo[sceneModelIndex].albedoTextureIndex;
    pushConstants.normalTextureIndex                        = subsurfaceScattering->modelsInfo[sceneModelIndex].normalTextureIndex;
    pushConstants.ormTextureIndex                           = subsurfaceScattering->modelsInfo[sceneModelIndex].ormTextureIndex;
    pushConstants.thicknessTextureIndex                     = subsurfaceScattering->modelsInfo[sceneModelIndex].thicknessTextureIndex;
    pushConstants.renderingLight                            = 0;
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdDraw(commandBuffer, mesh->triangleCount * 3, 1, 0, 0);

    if (renderLightSpheres) {
        pushConstants.viewModelMatrix = subsurfaceScattering->viewMatrix; // no model matrix needed here
        pushConstants.renderingLight  = 1;
        pushConstants.vertexOffset    = sphereMesh->indexOffset;
        pushConstants.vertexCount     = sphereMesh->triangleCount * 3;
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDraw(commandBuffer, sphereMesh->triangleCount * 3 * 2, 1, 0, 0);
    }

    return true;
}

bool __avdSceneRenderModels(
    VkCommandBuffer commandBuffer,
    AVD_SceneSubsurfaceScattering *subsurfaceScattering,
    VkPipelineLayout pipelineLayout,
    AVD_Float time,
    bool renderLightSpheres)
{
    for (uint32_t i = 0; i < AVD_ARRAY_COUNT(subsurfaceScattering->modelsInfo); i++) {
        AVD_CHECK(__avdSceneRenderFirstMesh(commandBuffer, subsurfaceScattering, pipelineLayout, time, i, renderLightSpheres));
    }

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

bool __avdSceneRenderGBufferPass(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    AVD_CHECK(avdBeginRenderPassWithFramebuffer(
        commandBuffer,
        &subsurfaceScattering->gBuffer,
        (VkClearValue[]){
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
            {.depthStencil = {1.0f, 0}},
        },
        5));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->gBufferPipeline);
    VkDescriptorSet descriptorSets[] = {
        subsurfaceScattering->set0,
        appState->vulkan.bindlessDescriptorSet};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->gBufferPipelineLayout, 0, 2, descriptorSets, 0, NULL);

    AVD_Float time = (AVD_Float)appState->framerate.currentTime;

    __avdSceneRenderModels(commandBuffer, subsurfaceScattering, subsurfaceScattering->gBufferPipelineLayout, time, true);

    AVD_CHECK(avdEndRenderPass(commandBuffer));

    return true;
}

bool __avdSceneRenderAOPass(VkCommandBuffer commandBuffer, AVD_SceneSubsurfaceScattering *subsurfaceScattering, AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(subsurfaceScattering != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    AVD_CHECK(avdBeginRenderPassWithFramebuffer(
        commandBuffer,
        &subsurfaceScattering->aoBuffer,
        (VkClearValue[]){
            {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
        },
        1));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->aoPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->aoPipelineLayout, 0, 1, &appState->vulkan.bindlessDescriptorSet, 0, NULL);

    AVD_SubSurfaceScatteringUberPushConstants pushConstants = {
        .projectionMatrix = subsurfaceScattering->projectionMatrix,
        .viewModelMatrix  = subsurfaceScattering->viewMatrix,
        .cameraPosition   = avdVec4FromVec3(subsurfaceScattering->cameraPosition, 1.0f),
        .screenSize       = avdVec4((AVD_Float)subsurfaceScattering->sceneWidth, (AVD_Float)subsurfaceScattering->sceneHeight, 1.0f, 1.0f),
        .renderingLight   = 0,
    };
    vkCmdPushConstants(commandBuffer, subsurfaceScattering->aoPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    AVD_CHECK(avdEndRenderPass(commandBuffer));

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
    VkDescriptorSet descriptorSets[] = {
        subsurfaceScattering->set0,
        appState->vulkan.bindlessDescriptorSet};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subsurfaceScattering->lightingPipelineLayout, 0, 2, descriptorSets, 0, NULL);

    AVD_Float time = (AVD_Float)appState->framerate.currentTime;

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
    AVD_SubSurfaceScatteringCompositePushConstants pushConstants = {
        .renderMode = subsurfaceScattering->renderMode,
    };
    vkCmdPushConstants(commandBuffer, subsurfaceScattering->compositePipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
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

    AVD_CHECK(__avdSceneRenderGBufferPass(commandBuffer, subsurfaceScattering, appState));
    AVD_CHECK(__avdSceneRenderAOPass(commandBuffer, subsurfaceScattering, appState));
    // AVD_CHECK(__avdSceneRenderLightingPass(commandBuffer, subsurfaceScattering, appState));
    // AVD_CHECK(__avdSceneRenderIrradianceDiffusionPass(commandBuffer, subsurfaceScattering, appState));
    AVD_CHECK(__avdSceneRenderCompositePass(commandBuffer, subsurfaceScattering, appState));
    AVD_CHECK(__avdSceneRenderBloomIfNeeded(commandBuffer, subsurfaceScattering, appState));

    return true;
}
