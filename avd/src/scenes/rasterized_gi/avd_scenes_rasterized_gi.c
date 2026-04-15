#include "scenes/rasterized_gi/avd_scenes_rasterized_gi.h"
#include "avd_application.h"
#include "math/avd_matrix_non_simd.h"
#include "scenes/avd_scenes.h"

typedef struct {
    AVD_Matrix4x4 modelMatrix;
    AVD_Matrix4x4 projectionMatrix;
    AVD_Matrix4x4 viewMatrix;

    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t textureIndex;
    uint32_t pad1;
} AVD_RasterizedGIUberPushConstants;

static AVD_SceneRasterizedGI *PRIV_avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_RASTERIZED_GI);
    return &scene->rasterizedGI;
}

static bool PRIV_avdSetupBuffer(
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
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Scene/RasterizedGI/Buffer"));
    AVD_CHECK(avdVulkanBufferUpload(
        vulkan,
        buffer,
        dataList->items,
        size));
    return true;
}

static uint32_t PRIV_avdFindTextureIndexFromHash(AVD_SceneRasterizedGI *rasterizedGI, uint32_t hash)
{
    AVD_ASSERT(rasterizedGI != NULL);

    for (uint32_t i = 0; i < rasterizedGI->imagesCount; i++) {
        if (rasterizedGI->imagesHashes[i] == hash) {
            return i;
        }
    }

    return 0; // as a fallback return the first texture
}

static bool PRIV_avdRenderModelNode(VkCommandBuffer commandBuffer, AVD_SceneRasterizedGI *rasterizedGI, AVD_ModelNode *node, AVD_Matrix4x4 parentTransform)
{
    AVD_ASSERT(rasterizedGI != NULL);
    AVD_ASSERT(node != NULL);
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);

    AVD_Matrix4x4 localTransform  = avdTransformToMatrix(&node->transform);
    AVD_Matrix4x4 globalTransform = avdMat4x4Multiply(parentTransform, localTransform);

    if (node->hasMesh) {
        AVD_RasterizedGIUberPushConstants pushConstants = {
            .projectionMatrix = rasterizedGI->projectionMatrix,
            .modelMatrix      = globalTransform,
            .viewMatrix       = rasterizedGI->viewMatrix,
            .vertexCount      = node->mesh.triangleCount * 3,
            .vertexOffset     = node->mesh.indexOffset,
            .textureIndex     = PRIV_avdFindTextureIndexFromHash(rasterizedGI, node->mesh.material.albedoTexture.id) + 1,
        };
        vkCmdPushConstants(commandBuffer, rasterizedGI->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDraw(commandBuffer, node->mesh.triangleCount * 3, 1, 0, 0);
    }

    for (AVD_UInt32 i = 0; i < AVD_MODEL_NODE_MAX_CHILDREN; i++) {
        if (node->children[i])
            AVD_CHECK(PRIV_avdRenderModelNode(commandBuffer, rasterizedGI, node->children[i], globalTransform));
    }

    return true;
}

bool avdSceneRasterizedGICheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    // Not really a dependency but we should ensure that its present
    AVD_FILE_INTEGRITY_CHECK("assets/scene_rasterized_gi/LICENSE");

    // The Model files
    AVD_FILE_INTEGRITY_CHECK("assets/scene_rasterized_gi/SM_Deccer_Cubes_Textured_Complex.gltf");

    // The images
    AVD_FILE_INTEGRITY_CHECK("assets/scene_rasterized_gi/T_Blue_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_rasterized_gi/T_Orange_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_rasterized_gi/T_Purple_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_rasterized_gi/T_Red_D.png");
    AVD_FILE_INTEGRITY_CHECK("assets/scene_rasterized_gi/T_LightGreen_D.png");

    return true;
}

bool avdSceneRasterizedGIRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneRasterizedGICheckIntegrity;
    api->init           = avdSceneRasterizedGIInit;
    api->render         = avdSceneRasterizedGIRender;
    api->update         = avdSceneRasterizedGIUpdate;
    api->destroy        = avdSceneRasterizedGIDestroy;
    api->load           = avdSceneRasterizedGILoad;
    api->inputEvent     = avdSceneRasterizedGIInputEvent;

    api->displayName = "Rasterized Global Illumination";
    api->id          = "DeccerCubes";

    return true;
}

bool avdSceneRasterizedGIInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRasterizedGI *rasterizedGI = PRIV_avdSceneGetTypePtr(scene);

    rasterizedGI->loadStage        = 0;
    rasterizedGI->imagesCount      = 0;
    rasterizedGI->projectionMatrix = avdMatPerspective(avdDeg2Rad(45.0f), 16.0f / 9.0f, 0.01f, 100.0f);
    rasterizedGI->viewMatrix       = avdMatLookAt(avdVec3(12.0f, 12.0f, 12.0f), avdVec3(0.0f, 0.0f, 0.0f), avdVec3(0.0f, 1.0f, 0.0f));

    avd3DSceneCreate(&rasterizedGI->scene);

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &rasterizedGI->set0Layout,
        appState->vulkan.device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 2,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT));
    AVD_CHECK(avdAllocateDescriptorSet(
        appState->vulkan.device,
        appState->vulkan.descriptorPool,
        rasterizedGI->set0Layout,
        &rasterizedGI->set0));
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, rasterizedGI->set0Layout, "[DescriptorSetLayout][Scene]:RasterizedGI/Set0");
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET, rasterizedGI->set0, "[DescriptorSet][Scene]:RasterizedGI/Set0");

    AVD_CHECK(avdRenderableTextCreate(
        &rasterizedGI->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        "Rasterized Global Illumination Demo",
        48.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &rasterizedGI->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "A simple GLTF scene with some weird transforms to test the transform calculation edge cases",
        18.0f));

    return true;
}

void avdSceneRasterizedGIDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRasterizedGI *rasterizedGI = PRIV_avdSceneGetTypePtr(scene);

    avdVulkanBufferDestroy(&appState->vulkan, &rasterizedGI->vertexBuffer);
    avdVulkanBufferDestroy(&appState->vulkan, &rasterizedGI->indexBuffer);

    vkDestroyDescriptorSetLayout(appState->vulkan.device, rasterizedGI->set0Layout, NULL);

    avd3DSceneDestroy(&rasterizedGI->scene);
    avdRenderableTextDestroy(&rasterizedGI->title, &appState->vulkan);
    avdRenderableTextDestroy(&rasterizedGI->info, &appState->vulkan);

    for (AVD_UInt32 i = 0; i < rasterizedGI->imagesCount; i++) {
        avdVulkanImageDestroy(&appState->vulkan, &rasterizedGI->images[i]);
    }

    vkDestroyPipelineLayout(appState->vulkan.device, rasterizedGI->pipelineLayout, NULL);
    vkDestroyPipeline(appState->vulkan.device, rasterizedGI->pipeline, NULL);
}

bool avdSceneRasterizedGILoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    AVD_SceneRasterizedGI *rasterizedGI = PRIV_avdSceneGetTypePtr(scene);

    switch (rasterizedGI->loadStage) {
        case 0:
            *statusMessage = "Loaded scene license";
        case 1:
            *statusMessage = "Loaded plain deccer cube model";
            AVD_CHECK(avd3DSceneLoadGltf("assets/scene_rasterized_gi/SM_Deccer_Cubes_Textured_Complex.gltf", &rasterizedGI->scene, AVD_GLT_LOAD_FLAG_NONE));
            break;
        case 2:
            *statusMessage = "Setup GPU Buffers";
            AVD_CHECK(PRIV_avdSetupBuffer(
                &appState->vulkan,
                &rasterizedGI->indexBuffer,
                &rasterizedGI->scene.modelResources.indicesList));
            AVD_CHECK(PRIV_avdSetupBuffer(
                &appState->vulkan,
                &rasterizedGI->vertexBuffer,
                &rasterizedGI->scene.modelResources.verticesList));
            VkWriteDescriptorSet descriptorSetWrite[2] = {0};
            AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite[0],
                                                  rasterizedGI->set0,
                                                  0,
                                                  &rasterizedGI->vertexBuffer.descriptorBufferInfo));
            AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite[1],
                                                  rasterizedGI->set0,
                                                  1,
                                                  &rasterizedGI->indexBuffer.descriptorBufferInfo));
            vkUpdateDescriptorSets(appState->vulkan.device, 2, descriptorSetWrite, 0, NULL);
            break;
        case 3:
            *statusMessage                                      = "Created pipelines...";
            AVD_VulkanPipelineCreationInfo pipelineCreationInfo = {0};
            avdPipelineUtilsPipelineCreationInfoInit(&pipelineCreationInfo);
            pipelineCreationInfo.enableDepthTest = true;

            AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
                &rasterizedGI->pipelineLayout,
                &rasterizedGI->pipeline,
                appState->vulkan.device,
                (VkDescriptorSetLayout[]){
                    rasterizedGI->set0Layout,
                    appState->vulkan.bindlessDescriptorSetLayout,
                },
                2,
                sizeof(AVD_RasterizedGIUberPushConstants),
                appState->renderer.sceneFramebuffer.renderPass,
                (AVD_UInt32)appState->renderer.sceneFramebuffer.colorAttachments.count,
                "DeccerCubeVert",
                "DeccerCubeFrag",
                NULL,
                &pipelineCreationInfo));
            break;
        case 4:
            *statusMessage                                                                  = "Loaded images...";
            VkWriteDescriptorSet descriptorSetWrites[AVD_ARRAY_COUNT(rasterizedGI->images)] = {0};
            AVD_Model *model                                                                = (AVD_Model *)avdListGet(&rasterizedGI->scene.modelsList, 0);
            for (AVD_UInt32 i = 0; i < model->meshes.count && rasterizedGI->imagesCount < AVD_ARRAY_COUNT(rasterizedGI->images); i++) {
                AVD_Mesh *mesh = (AVD_Mesh *)avdListGet(&model->meshes, i);
                if (!mesh->material.albedoTexture.hasTexture)
                    continue;
                rasterizedGI->imagesHashes[rasterizedGI->imagesCount] = mesh->material.albedoTexture.id;
                AVD_LOG_INFO("Loading image: %s (hash: %u)", mesh->material.albedoTexture.path, mesh->material.albedoTexture.id);
                AVD_CHECK(avdVulkanImageLoadFromFile(
                    &appState->vulkan,
                    mesh->material.albedoTexture.path,
                    &rasterizedGI->images[rasterizedGI->imagesCount], NULL));

                VkWriteDescriptorSet *write = &descriptorSetWrites[rasterizedGI->imagesCount];
                write->sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write->dstSet               = appState->vulkan.bindlessDescriptorSet;
                write->descriptorCount      = 1;
                write->dstBinding           = (uint32_t)AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                write->descriptorType       = avdVulkanToVkDescriptorType(AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                write->dstArrayElement      = rasterizedGI->imagesCount + 1;
                write->pImageInfo           = &rasterizedGI->images[rasterizedGI->imagesCount].defaultSubresource.descriptorImageInfo;

                rasterizedGI->imagesCount += 1;
            }
            vkUpdateDescriptorSets(appState->vulkan.device, rasterizedGI->imagesCount, descriptorSetWrites, 0, NULL);
            AVD_LOG_INFO("Loaded all %d textures", rasterizedGI->imagesCount);
        case 5:
            *statusMessage = "Done loading...";
            avd3DSceneDebugLog(&rasterizedGI->scene, "Rasterized Global Illumination");
            break;
        default:
            AVD_LOG_ERROR("Rasterized Global Illumination scene invalid load stage");
            return false;
    }

    rasterizedGI->loadStage++;
    *progress = (float)rasterizedGI->loadStage / 5.0f;

    return *progress > 1.0f;
}

void avdSceneRasterizedGIInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
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

bool avdSceneRasterizedGIUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRasterizedGI *rasterizedGI = PRIV_avdSceneGetTypePtr(scene);

    // rotate the camera around the orgin
    rasterizedGI->viewMatrix = avdMatLookAt(
        avdVec3(16.0f * cosf((AVD_Float)appState->framerate.currentTime * 0.2f), 16.0f, 16.0f * sinf((AVD_Float)appState->framerate.currentTime * 0.2f)),
        avdVec3(0.0f, 0.0f, 0.0f),
        avdVec3(0.0f, 1.0f, 0.0f));

    return true;
}

bool avdSceneRasterizedGIRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneRasterizedGI *rasterizedGI = PRIV_avdSceneGetTypePtr(scene);

    VkCommandBuffer commandBuffer = avdVulkanRendererGetCurrentCmdBuffer(&appState->renderer);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));
    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][Scene]:RasterizedGI/Render");

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rasterizedGI->pipeline);
    VkDescriptorSet descriptorSets[] = {
        rasterizedGI->set0,
        appState->vulkan.bindlessDescriptorSet,
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rasterizedGI->pipelineLayout, 0, 2, descriptorSets, 0, NULL);

    AVD_Model *model = (AVD_Model *)avdListGet(&rasterizedGI->scene.modelsList, 0);
    AVD_CHECK(PRIV_avdRenderModelNode(commandBuffer, rasterizedGI, model->mainScene, avdMat4x4Identity()));

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&rasterizedGI->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&rasterizedGI->info, &infoWidth, &infoHeight);

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &rasterizedGI->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &rasterizedGI->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);
    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}
