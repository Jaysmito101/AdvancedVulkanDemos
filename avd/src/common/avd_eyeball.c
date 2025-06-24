#include "common/avd_eyeball.h"

typedef struct {
    AVD_Vector4 eyePosition;
    AVD_Vector4 eyeRotation;
    AVD_Vector4 cameraPosition;
    AVD_Vector4 cameraDirection;
    AVD_Float radius;
} AVD_EyeballPushConstants;

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
        },
        2,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT));

    AVD_CHECK(avdAllocateDescriptorSet(
        vulkan->device,
        vulkan->descriptorPool,
        eyeball->descriptorSetLayout,
        &eyeball->descriptorSet));
    VkWriteDescriptorSet descriptorSetWrite[2] = {0};
    AVD_CHECK(avdWriteUniformBufferDescriptorSet(&descriptorSetWrite[0],
                                                 eyeball->descriptorSet,
                                                 0,
                                                 &eyeball->configBuffer.descriptorBufferInfo));
    AVD_CHECK(avdWriteImageDescriptorSet(&descriptorSetWrite[1],
                                         eyeball->descriptorSet,
                                         1,
                                         &eyeball->veinsTexture.descriptorImageInfo));
    vkUpdateDescriptorSets(vulkan->device, 2, &descriptorSetWrite[0], 0, NULL);
    return true;
}

bool __avdEyeballSetupDefaults(AVD_Eyeball *eyeball)
{
    AVD_ASSERT(eyeball != NULL);

    eyeball->uniformValue.lightPosition[0] = avdVec4(0.0f, 10.0f, 10.0f, 1.0f);
    eyeball->uniformValue.lightColor[0] = avdVec4(1.0f, 1.0f, 1.0f, 1.0f);
    eyeball->uniformValue.lightCount = 1;
    eyeball->uniformsDirty = true;

    return true;
}

bool avdEyeballCreate(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan, VkRenderPass renderPass, uint32_t attachmentCount)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_CHECK(avdVulkanBufferCreate(
        vulkan,
        &eyeball->configBuffer,
        sizeof(AVD_EyeballUniforms),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    AVD_CHECK(avdVulkanImageCreate(
        vulkan,
        &eyeball->veinsTexture,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        512, 512));
    AVD_CHECK(__avdEyeballSetupDescriptors(eyeball, vulkan));
    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &eyeball->pipelineLayout,
        &eyeball->pipeline,
        vulkan->device,
        &eyeball->descriptorSetLayout,
        1,
        sizeof(AVD_EyeballPushConstants),
        renderPass,
        attachmentCount,
        "FullScreenQuadVert",
        "EyeballFrag",
        NULL));
    AVD_CHECK(__avdEyeballSetupDefaults(eyeball));

    return true;
}

void avdEyeballDestroy(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(vulkan != NULL);

    vkDestroyDescriptorSetLayout(vulkan->device, eyeball->descriptorSetLayout, NULL);

    vkDestroyPipelineLayout(vulkan->device, eyeball->pipelineLayout, NULL);
    vkDestroyPipeline(vulkan->device, eyeball->pipeline, NULL);

    avdVulkanImageDestroy(vulkan, &eyeball->veinsTexture);
    avdVulkanBufferDestroy(vulkan, &eyeball->configBuffer);
}

void avdEyeballSetLights(
    AVD_Eyeball *eyeball,
    AVD_Vector3 *lightViewSpacePosition,
    AVD_Vector3 *lightColor,
    AVD_UInt32 lightCount)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(lightViewSpacePosition != NULL);
    AVD_ASSERT(lightColor != NULL);
    AVD_ASSERT(lightCount <= AVD_EYEBALL_MAX_LIGHTS);

    eyeball->uniformValue.lightCount = lightCount;
    for (AVD_UInt32 i = 0; i < lightCount; i++) {
        eyeball->uniformValue.lightPosition[i] = avdVec4(
            lightViewSpacePosition[i].x,
            lightViewSpacePosition[i].y,
            lightViewSpacePosition[i].z,
            1.0f
        );
        eyeball->uniformValue.lightColor[i] = avdVec4(
            lightColor[i].x,
            lightColor[i].y,
            lightColor[i].z,
            1.0f
        );
    }
    eyeball->uniformsDirty = true;
}

bool avdEyeballRecalculateVeinsTexture(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(eyeball != NULL);
    AVD_ASSERT(vulkan != NULL);

    return true; // Return true if successful
}

bool avdEyeballRender(
    VkCommandBuffer commandBuffer,
    AVD_Eyeball *eyeball,
    AVD_Vulkan *vulkan,
    AVD_Float radius,
    AVD_Vector3 cameraPosition,
    AVD_Vector3 cameraDirection,
    AVD_Vector3 eyePosition,
    AVD_Vector3 eyeRotation)
{
    AVD_ASSERT(commandBuffer != VK_NULL_HANDLE);
    AVD_ASSERT(eyeball != NULL);

    if (eyeball->uniformsDirty) {
        AVD_CHECK(avdVulkanBufferUpload(
            vulkan,
            &eyeball->configBuffer,
            &eyeball->uniformValue,
            sizeof(AVD_EyeballUniforms)));
        eyeball->uniformsDirty = false;
    }

    AVD_EyeballPushConstants pushConstants = {
        .eyePosition = avdVec4FromVec3(eyePosition, 1.0f),
        .eyeRotation = avdVec4FromVec3(eyeRotation, 0.0f),
        .cameraPosition = avdVec4FromVec3(cameraPosition, 1.0f),
        .cameraDirection = avdVec4FromVec3(cameraDirection, 0.0f),
        .radius      = radius
    };
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, eyeball->pipeline);
    vkCmdPushConstants(commandBuffer, eyeball->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, eyeball->pipelineLayout, 0, 1, &eyeball->descriptorSet, 0, NULL);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    return true;
}