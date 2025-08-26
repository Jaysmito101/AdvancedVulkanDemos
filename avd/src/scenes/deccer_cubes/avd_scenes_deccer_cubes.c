#include "scenes/deccer_cubes/avd_scenes_deccer_cubes.h"
#include "avd_application.h"
#include "scenes/avd_scenes.h"


typedef struct {
    AVD_Matrix4x4 viewModelMatrix;
    AVD_Matrix4x4 projectionMatrix;

    int32_t vertexOffset;
    int32_t vertexCount;
    int32_t pad0;
    int32_t pad1;
} AVD_DeccerCubeUberPushConstants;

static AVD_SceneDeccerCubes *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_DECCER_CUBES);
    return &scene->deccerCubes;
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

static bool __avdRenderModelNode(VkCommandBuffer commandBuffer, AVD_SceneDeccerCubes *deccerCubes, AVD_ModelNode *node, AVD_Matrix4x4 parentTransform)
{
    AVD_ASSERT(deccerCubes != NULL);
    AVD_ASSERT(node != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    AVD_Matrix4x4 localTransform  = avdTransformToMatrix(&node->transform);
    AVD_Matrix4x4 globalTransform = avdMat4x4Multiply(parentTransform, localTransform);

    if (node->hasMesh) {
        AVD_DeccerCubeUberPushConstants pushConstants = {
            .projectionMatrix = deccerCubes->projectionMatrix,
            .viewModelMatrix  = globalTransform,
            .vertexCount      = node->mesh.triangleCount * 3,
            .vertexOffset     = node->mesh.indexOffset,
        };
        vkCmdPushConstants(commandBuffer, deccerCubes->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDraw(commandBuffer, node->mesh.triangleCount * 3, 1, 0, 0);
    }

    for (AVD_UInt32 i = 0; i < AVD_MODEL_NODE_MAX_CHILDREN; i++) {
        if (node->children[i])
            AVD_CHECK(__avdRenderModelNode(commandBuffer, deccerCubes, node->children[i], globalTransform));
    }

    return true;
}

bool avdSceneDeccerCubesCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // Not really a dependency but we should ensure that its present
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/LICENSE");

    // The Model files
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/SM_Deccer_Cubes_Textured_Complex.gltf");

    // The images
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Blue_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Orange_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Purple_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_Red_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_deccer_cubes/T_LightGreen_D.png");

    return true;
}

bool avdSceneDeccerCubesRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneDeccerCubesCheckIntegrity;
    api->init           = avdSceneDeccerCubesInit;
    api->render         = avdSceneDeccerCubesRender;
    api->update         = avdSceneDeccerCubesUpdate;
    api->destroy        = avdSceneDeccerCubesDestroy;
    api->load           = avdSceneDeccerCubesLoad;
    api->inputEvent     = avdSceneDeccerCubesInputEvent;

    api->displayName = "Deccer Cubes";
    api->id          = "DDGIPlaceholder";

    return true;
}

bool avdSceneDeccerCubesInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneDeccerCubes *deccerCubes = __avdSceneGetTypePtr(scene);

    deccerCubes->loadStage        = 0;
    deccerCubes->projectionMatrix = avdMatPerspective(avdDeg2Rad(45.0f), 16.0f / 9.0f, 0.01f, 100.0f);
    deccerCubes->viewMatrix       = avdMatLookAt(avdVec3(12.0f, 12.0f, 12.0f), avdVec3(0.0f, 0.0f, 0.0f), avdVec3(0.0f, 1.0f, 0.0f));

    avd3DSceneCreate(&deccerCubes->scene);

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &deccerCubes->set0Layout,
        appState->vulkan.device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 2,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT));
    AVD_CHECK(avdAllocateDescriptorSet(
        appState->vulkan.device,
        appState->vulkan.descriptorPool,
        deccerCubes->set0Layout,
        &deccerCubes->set0));

    AVD_CHECK(avdRenderableTextCreate(
        &deccerCubes->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Deccer Cubes Demo",
        48.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &deccerCubes->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "A simple GLTF scene with some weird transforms to test the transform calculation edge cases",
        18.0f));

    return true;
}

void avdSceneDeccerCubesDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneDeccerCubes *deccerCubes = __avdSceneGetTypePtr(scene);

    avdVulkanBufferDestroy(&appState->vulkan, &deccerCubes->vertexBuffer);
    avdVulkanBufferDestroy(&appState->vulkan, &deccerCubes->indexBuffer);

    vkDestroyDescriptorSetLayout(appState->vulkan.device, deccerCubes->set0Layout, NULL);

    avd3DSceneDestroy(&deccerCubes->scene);
    avdRenderableTextDestroy(&deccerCubes->title, &appState->vulkan);
    avdRenderableTextDestroy(&deccerCubes->info, &appState->vulkan);

    vkDestroyPipelineLayout(appState->vulkan.device, deccerCubes->pipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, deccerCubes->pipeline, NULL);
}

bool avdSceneDeccerCubesLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    AVD_SceneDeccerCubes *deccerCubes = __avdSceneGetTypePtr(scene);

    switch (deccerCubes->loadStage) {
        case 0:
            *statusMessage = "Loaded scene license";
        case 1:
            *statusMessage = "Loaded plain deccer cube model";
            AVD_CHECK(avd3DSceneLoadGltf("assets/scene_deccer_cubes/SM_Deccer_Cubes_Textured_Complex.gltf", &deccerCubes->scene, AVD_GLT_LOAD_FLAG_NONE));
            break;
        case 2:
            *statusMessage = "Setup GPU Buffers";
            AVD_CHECK(__avdSetupBuffer(
                &appState->vulkan,
                &deccerCubes->indexBuffer,
                &deccerCubes->scene.modelResources.indicesList));
            AVD_CHECK(__avdSetupBuffer(
                &appState->vulkan,
                &deccerCubes->vertexBuffer,
                &deccerCubes->scene.modelResources.verticesList));
            VkWriteDescriptorSet descriptorSetWrite[2] = {0};
            AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite[0],
                                                  deccerCubes->set0,
                                                  0,
                                                  &deccerCubes->vertexBuffer.descriptorBufferInfo));
            AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite[1],
                                                  deccerCubes->set0,
                                                  1,
                                                  &deccerCubes->indexBuffer.descriptorBufferInfo));
            vkUpdateDescriptorSets(appState->vulkan.device, 2, descriptorSetWrite, 0, NULL);
            break;
        case 3:
            *statusMessage                                      = "Created pipelines...";
            AVD_VulkanPipelineCreationInfo pipelineCreationInfo = {0};
            avdPipelineUtilsPipelineCreationInfoInit(&pipelineCreationInfo);
            pipelineCreationInfo.enableDepthTest = true;

            AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
                &deccerCubes->pipelineLayout,
                &deccerCubes->pipeline,
                appState->vulkan.device,
                (VkDescriptorSetLayout[]){
                    deccerCubes->set0Layout,
                },
                1,
                sizeof(AVD_DeccerCubeUberPushConstants),
                appState->renderer.sceneFramebuffer.renderPass,
                (AVD_UInt32)appState->renderer.sceneFramebuffer.colorAttachments.count,
                "DeccerCubeVert",
                "DeccerCubeFrag",
                NULL,
                &pipelineCreationInfo));
            break;
        case 4:
            *statusMessage = "Done loading...";
            avd3DSceneDebugLog(&deccerCubes->scene, "Deccer Cubes");
            break;
        default:
            AVD_LOG("Deccer Cubes scene invalid load stage");
            return false;
    }

    deccerCubes->loadStage++;
    *progress = (float)deccerCubes->loadStage / 4.0f;

    return *progress > 1.0f;
}

void avdSceneDeccerCubesInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        }
    }
}

bool avdSceneDeccerCubesUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneDeccerCubes *deccerCubes = __avdSceneGetTypePtr(scene);

    // rotate the camera around the orgin
    deccerCubes->viewMatrix = avdMatLookAt(
        avdVec3(16.0f * cosf((AVD_Float)appState->framerate.currentTime * 0.2f), 16.0f, 16.0f * sinf((AVD_Float)appState->framerate.currentTime * 0.2f)),
        avdVec3(0.0f, 0.0f, 0.0f),
        avdVec3(0.0f, 1.0f, 0.0f));

    return true;
}

bool avdSceneDeccerCubesRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneDeccerCubes *deccerCubes = __avdSceneGetTypePtr(scene);

    VkCommandBuffer commandBuffer = appState->renderer.resources[appState->renderer.currentFrameIndex].commandBuffer;

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deccerCubes->pipeline);
    VkDescriptorSet descriptorSets[] = {
        deccerCubes->set0,
        appState->vulkan.bindlessDescriptorSet};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deccerCubes->pipelineLayout, 0, 1, descriptorSets, 0, NULL);

    AVD_Model* model = (AVD_Model*)avdListGet(&deccerCubes->scene.modelsList, 0);
    AVD_CHECK(__avdRenderModelNode(commandBuffer, deccerCubes, model->mainScene, deccerCubes->viewMatrix));

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&deccerCubes->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&deccerCubes->info, &infoWidth, &infoHeight);

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &deccerCubes->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &deccerCubes->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}