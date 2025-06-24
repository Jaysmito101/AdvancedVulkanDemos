#ifndef AVD_EYEBALL_H
#define AVD_EYEBALL_H

#include "core/avd_core.h"
#include "math/avd_math.h"
#include "vulkan/avd_vulkan.h"

#ifndef AVD_EYEBALL_MAX_LIGHTS
#define AVD_EYEBALL_MAX_LIGHTS 16
#endif

typedef struct {
    AVD_Vector4 lightPosition[AVD_EYEBALL_MAX_LIGHTS];
    AVD_Vector4 lightColor[AVD_EYEBALL_MAX_LIGHTS];
    uint32_t lightCount;
} AVD_EyeballUniforms;

typedef struct AVD_Eyeball {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    AVD_VulkanImage veinsTexture;

    AVD_EyeballUniforms uniformValue;
    bool uniformsDirty;

    AVD_VulkanBuffer configBuffer;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
} AVD_Eyeball;

bool avdEyeballCreate(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan, VkRenderPass renderPass, uint32_t attachmentCount);
void avdEyeballDestroy(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan);
bool avdEyeballRecalculateVeinsTexture(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan);
void avdEyeballSetLights(
    AVD_Eyeball *eyeball,
    AVD_Vector3 *lightViewSpacePosition,
    AVD_Vector3 *lightColor,
    AVD_UInt32 lightCount);
bool avdEyeballRender(
    VkCommandBuffer commandBuffer,
    AVD_Eyeball *eyeball,
    AVD_Vulkan *vulkan,
    AVD_Float radius,
    AVD_Vector3 cameraPosition,
    AVD_Vector3 cameraDirection,
    AVD_Vector3 eyePosition,
    AVD_Vector3 eyeRotation);

#endif // AVD_EYEBALL_H