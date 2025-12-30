#include "common/avd_eyeball.h"
#include "model/avd_model.h"

typedef struct {
    AVD_Vector4 eyePosition;
    AVD_Vector4 eyeRotation;
    AVD_Matrix4x4 cameraMatrix;
    AVD_Float radius;
} AVD_EyeballUniforms;

bool __avdEyeballSetupDescriptors(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &eyeball->descriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // Uniforms
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Veins texture
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         // Vertex buffer
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER          // Index buffer
        },
        4,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT));

    AVD_CHECK(avdAllocateDescriptorSet(
        vulkan->device,
        vulkan->descriptorPool,
        eyeball->descriptorSetLayout,
        &eyeball->descriptorSet));
    VkWriteDescriptorSet descriptorSetWrite[4] = {0};
    AVD_CHECK(avdWriteUniformBufferDescriptorSet(&descriptorSetWrite[0],
                                                 eyeball->descriptorSet,
                                                 0,
                                                 &eyeball->configBuffer.descriptorBufferInfo));
    AVD_CHECK(avdWriteImageDescriptorSet(&descriptorSetWrite[1],
                                         eyeball->descriptorSet,
                                         1,
                                         &eyeball->veinsTexture.defaultSubresource.descriptorImageInfo));
    AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite[2],
                                          eyeball->descriptorSet,
                                          2,
                                          &eyeball->vertexBuffer.descriptorBufferInfo));
    AVD_CHECK(avdWriteBufferDescriptorSet(&descriptorSetWrite[3],
                                          eyeball->descriptorSet,
                                          3,
                                          &eyeball->indexBuffer.descriptorBufferInfo));
    vkUpdateDescriptorSets(vulkan->device, 4, &descriptorSetWrite[0], 0, NULL);
    return true;
}

bool __avdEyeballSetupDefaults(AVD_Eyeball *eyeball)
{
    AVD_ASSERT(eyeball != NULL);

    return true;
}

bool __avdEyeballSetupMesh(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_3DScene eyeballScene = {0};
    AVD_CHECK(avd3DSceneCreate(&eyeballScene));
    AVD_Model model;
    AVD_CHECK(avdModelCreate(&model, 0));
    AVD_CHECK(avdModelAddOctaSphere(
        &model,
        &eyeballScene.modelResources,
        "Eyeball/Sclera",
        0,
        1.0f,
        5));
    AVD_CHECK(avdModelAddOctaSphere(
        &model,
        &eyeballScene.modelResources,
        "Eyeball/Lens",
        1,
        0.8f,
        5));
    model.id = 0;
    snprintf(model.name, sizeof(model.name), "Eyeball");
    AVD_CHECK(avd3DSceneAddModel(&eyeballScene, &model));
    AVD_LOG_INFO("Eyeball model:");
    avd3DSceneDebugLog(&eyeballScene, "Eyeball");

    memcpy(&eyeball->scleraMesh, avdListGet(&model.meshes, 0), sizeof(AVD_Mesh));
    memcpy(&eyeball->lensMesh, avdListGet(&model.meshes, 1), sizeof(AVD_Mesh));

    // upload vertices and indices to the vertex and index buffers
    AVD_CHECK(avdVulkanBufferUpload(
        vulkan,
        &eyeball->vertexBuffer,
        eyeballScene.modelResources.verticesList.items,
        sizeof(AVD_ModelVertexPacked) * eyeballScene.modelResources.verticesList.count));
    AVD_CHECK(avdVulkanBufferUpload(
        vulkan,
        &eyeball->indexBuffer,
        eyeballScene.modelResources.indicesList.items,
        sizeof(uint32_t) * eyeballScene.modelResources.indicesList.count));
    avd3DSceneDestroy(&eyeballScene);

    return true;
}

bool avdEyeballCreate(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_CHECK(avdVulkanBufferCreate(
        vulkan,
        &eyeball->configBuffer,
        sizeof(AVD_EyeballUniforms),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Scene/Eyeball/ConfigBuffer"));
    AVD_CHECK(avdVulkanBufferCreate(
        vulkan,
        &eyeball->vertexBuffer,
        sizeof(AVD_ModelVertexPacked) * AVD_EYEBALL_MAX_VERTICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Scene/Eyeball/VertexBuffer"));
    AVD_CHECK(avdVulkanBufferCreate(
        vulkan,
        &eyeball->indexBuffer,
        sizeof(uint32_t) * AVD_EYEBALL_MAX_VERTICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Scene/Eyeball/IndexBuffer"));
    AVD_CHECK(avdVulkanImageCreate(
        vulkan,
        &eyeball->veinsTexture,
        avdVulkanImageGetDefaultCreateInfo(
            512, 512,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)));
    AVD_CHECK(__avdEyeballSetupDescriptors(eyeball, vulkan));
    AVD_CHECK(__avdEyeballSetupDefaults(eyeball));
    AVD_CHECK(__avdEyeballSetupMesh(eyeball, vulkan));

    return true;
}

void avdEyeballDestroy(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(vulkan != NULL);

    vkDestroyDescriptorSetLayout(vulkan->device, eyeball->descriptorSetLayout, NULL);

    avdVulkanImageDestroy(vulkan, &eyeball->veinsTexture);
    avdVulkanBufferDestroy(vulkan, &eyeball->vertexBuffer);
    avdVulkanBufferDestroy(vulkan, &eyeball->indexBuffer);
    avdVulkanBufferDestroy(vulkan, &eyeball->configBuffer);
}

bool avdEyeballRecalculateVeinsTexture(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(vulkan != NULL);

    return true; // Return true if successful
}
