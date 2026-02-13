#include "scenes/realistic_head/avd_scenes_realistic_head.h"
#include "avd_application.h"
#include "core/avd_base.h"
#include "model/avd_model.h"
#include "model/avd_model_base.h"

typedef struct {
    AVD_Matrix4x4 viewModelMatrix;
    AVD_Matrix4x4 projectionMatrix;

    int32_t indexOffset;
    int32_t indexCount;
    int32_t pad0;
    int32_t pad1;
} AVD_RealisticHeadPushConstants;

static AVD_SceneRealisticHead *__avdSceneGetTypePtr(union AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_REALISTIC_HEAD);
    return &scene->realisticHead;
}

static bool __avdSetupBuffer(
    AVD_Vulkan *vulkan,
    AVD_VulkanBuffer *buffer,
    AVD_List *dataList)
{
    AVD_Size size = dataList->count * dataList->itemSize;
    AVD_CHECK(avdVulkanBufferCreate(
        vulkan,
        buffer,
        size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    AVD_CHECK(avdVulkanBufferUpload(
        vulkan,
        buffer,
        dataList->items,
        size));
    return true;
}

bool avdSceneRealisticHeadCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    AVD_FILE_INTEGRITY_CHECK("assets/scene_realistic_head/head.glb");
    return true;
}

bool avdSceneRealisticHeadRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneRealisticHeadCheckIntegrity;
    api->init           = avdSceneRealisticHeadInit;
    api->render         = avdSceneRealisticHeadRender;
    api->update         = avdSceneRealisticHeadUpdate;
    api->destroy        = avdSceneRealisticHeadDestroy;
    api->load           = avdSceneRealisticHeadLoad;
    api->inputEvent     = avdSceneRealisticHeadInputEvent;

    api->displayName = "Realistic Head";
    api->id          = "StaticSubsurfaceScattering";

    return true;
}

bool avdSceneRealisticHeadInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    realisticHead->loadStage = 0;

    AVD_CHECK(avd3DSceneCreate(&realisticHead->models));

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &realisticHead->set0Layout,
        appState->vulkan.device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 2,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT));
    AVD_CHECK(avdAllocateDescriptorSet(
        appState->vulkan.device,
        appState->vulkan.descriptorPool,
        realisticHead->set0Layout,
        &realisticHead->set0));

    // Initialize title and info text
    AVD_CHECK(avdRenderableTextCreate(
        &realisticHead->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Realistic Head",
        64.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &realisticHead->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Rendering a nice looking human head!",
        24.0f));

    realisticHead->sceneWidth  = (float)appState->renderer.sceneFramebuffer.width;
    realisticHead->sceneHeight = (float)appState->renderer.sceneFramebuffer.height;

    return true;
}

void avdSceneRealisticHeadDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    avd3DSceneDestroy(&realisticHead->models);

    vkDestroyDescriptorSetLayout(appState->vulkan.device, realisticHead->set0Layout, NULL);

    vkDestroyPipeline(appState->vulkan.device, realisticHead->pipeline, NULL);
    vkDestroyPipelineLayout(appState->vulkan.device, realisticHead->pipelineLayout, NULL);

    avdVulkanBufferDestroy(&appState->vulkan, &realisticHead->vertexBuffer);
    avdVulkanBufferDestroy(&appState->vulkan, &realisticHead->indexBuffer);

    avdRenderableTextDestroy(&realisticHead->title, &appState->vulkan);
    avdRenderableTextDestroy(&realisticHead->info, &appState->vulkan);
}

bool avdSceneRealisticHeadLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    switch (realisticHead->loadStage) {
        case 0:
            *statusMessage = "Starting to load Realistic Head scene...";
            break;
        case 1:
            *statusMessage = "Loaded 3D model.";
            AVD_CHECK(avd3DSceneLoadGltf("assets/scene_realistic_head/head.glb", &realisticHead->models, AVD_GLT_LOAD_FLAG_NONE));
            break;
        case 2:
            avd3DSceneDebugLog(&realisticHead->models, "RealisticHead/Models");
            break;
        case 3:
            *statusMessage = "Setup GPU Buffers";
            AVD_CHECK(__avdSetupBuffer(
                &appState->vulkan,
                &realisticHead->indexBuffer,
                &realisticHead->models.modelResources.indicesList));
            AVD_CHECK(__avdSetupBuffer(
                &appState->vulkan,
                &realisticHead->vertexBuffer,
                &realisticHead->models.modelResources.verticesList));
            VkWriteDescriptorSet descriptorSetWrite[2] = {0};
            AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite[0],
                                                  realisticHead->set0,
                                                  0,
                                                  &realisticHead->vertexBuffer.descriptorBufferInfo));
            AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite[1],
                                                  realisticHead->set0,
                                                  1,
                                                  &realisticHead->indexBuffer.descriptorBufferInfo));
            vkUpdateDescriptorSets(appState->vulkan.device, 2, descriptorSetWrite, 0, NULL);
            break;
        case 4:
            *statusMessage = "Created Pipelines...";

            AVD_VulkanPipelineCreationInfo pipelineCreationInfo = {0};
            avdPipelineUtilsPipelineCreationInfoInit(&pipelineCreationInfo);
            pipelineCreationInfo.enableDepthTest = true;

            AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
                &realisticHead->pipelineLayout,
                &realisticHead->pipeline,
                appState->vulkan.device,
                (VkDescriptorSetLayout[]){
                    realisticHead->set0Layout,
                    appState->vulkan.bindlessDescriptorSetLayout,
                },
                2,
                sizeof(AVD_RealisticHeadPushConstants),
                appState->renderer.sceneFramebuffer.renderPass,
                (AVD_UInt32)appState->renderer.sceneFramebuffer.colorAttachments.count,
                "RealisticHeadVert",
                "RealisticHeadFrag",
                NULL,
                &pipelineCreationInfo));
            break;
        default:
            AVD_LOG("Realistic Head scene load stage invalid: %d", realisticHead->loadStage);
            break;
    }

    realisticHead->loadStage++;
    *progress = (float)realisticHead->loadStage / 4.0f;
    return *progress > 1.0f;
}

void avdSceneRealisticHeadInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(event != NULL);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        }
    }
}

static bool __avdSceneUpdateCamera(AVD_SceneRealisticHead *realisticHead, AVD_Float timer)
{
    AVD_ASSERT(realisticHead != NULL);

    timer         = timer * 0.1f;
    AVD_Float rad = 0.4f;

    realisticHead->viewModelMatrix = avdMatLookAt(
        avdVec3(rad * sinf(timer), 1.6f, rad * cosf(timer)), // Camera position
        avdVec3(0.0f, 1.62f, 0.0f),                          // Look at the origin
        avdVec3(0.0f, 1.0f, 0.0f)                            // Up vector
    );

    realisticHead->projectionMatrix = avdMatPerspective(
        avdDeg2Rad(67.0f),
        realisticHead->sceneWidth / realisticHead->sceneHeight,
        0.1f,
        100.0f);

    return true;
}

bool avdSceneRealisticHeadUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    AVD_CHECK(__avdSceneUpdateCamera(realisticHead, (float)appState->framerate.currentTime));

    return true;
}

bool __avdRenderModelNodeToGBuffer(VkCommandBuffer commandBuffer, AVD_ModelNode *node, AVD_Model *model, AVD_SceneRealisticHead *realisticHead)
{
    AVD_ASSERT(commandBuffer != NULL);
    AVD_ASSERT(node != NULL);
    AVD_ASSERT(model != NULL);
    AVD_ASSERT(realisticHead != NULL);

    AVD_ModelNode *eyes = model->mainScene->children[0];
    AVD_ModelNode *head = model->mainScene->children[1];

    if (node->hasMesh) {
        AVD_RealisticHeadPushConstants pushConstants = {
            .projectionMatrix = realisticHead->projectionMatrix,
            .viewModelMatrix  = realisticHead->viewModelMatrix,
            .indexCount       = node->mesh.triangleCount * 3,
            .indexOffset      = node->mesh.indexOffset,
        };
        vkCmdPushConstants(
            commandBuffer,
            realisticHead->pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(AVD_RealisticHeadPushConstants),
            &pushConstants);
        vkCmdDraw(commandBuffer, pushConstants.indexCount, 1, 0, 0);
    }

    for (AVD_UInt32 i = 0; i < AVD_MODEL_NODE_MAX_CHILDREN; i++) {
        if (node->children[i]) {
            AVD_CHECK(__avdRenderModelNodeToGBuffer(commandBuffer, node->children[i], model, realisticHead));
        }
    }

    return true;
}
bool avdSceneRealisticHeadRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    VkCommandBuffer commandBuffer         = avdVulkanRendererGetCurrentCmdBuffer(&appState->renderer);
    AVD_SceneRealisticHead *realisticHead = __avdSceneGetTypePtr(scene);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, realisticHead->pipeline);
    VkDescriptorSet descriptorSets[] = {
        realisticHead->set0,
        appState->vulkan.bindlessDescriptorSet,
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, realisticHead->pipelineLayout, 0, 2, descriptorSets, 0, NULL);

    AVD_Model *model = (AVD_Model *)avdListGet(&realisticHead->models.modelsList, 0);
    AVD_CHECK(__avdRenderModelNodeToGBuffer(commandBuffer, model->rootNode, model, realisticHead));

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&realisticHead->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&realisticHead->info, &infoWidth, &infoHeight);

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &realisticHead->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &realisticHead->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}
