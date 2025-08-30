#ifndef AVD_EYEBALL_H
#define AVD_EYEBALL_H

#include "core/avd_core.h"
#include "math/avd_math.h"
#include "model/avd_3d_scene.h"
#include "vulkan/avd_vulkan.h"

#ifndef AVD_EYEBALL_MAX_LIGHTS
#define AVD_EYEBALL_MAX_LIGHTS 16
#endif

#ifndef AVD_EYEBALL_MAX_VERTICES
#define AVD_EYEBALL_MAX_VERTICES 1024 * 1024
#endif

typedef struct AVD_Eyeball {
    AVD_VulkanImage veinsTexture;

    AVD_Mesh scleraMesh;
    AVD_Mesh lensMesh;

    AVD_VulkanBuffer vertexBuffer;
    AVD_VulkanBuffer indexBuffer;
    AVD_VulkanBuffer configBuffer;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
} AVD_Eyeball;

bool avdEyeballCreate(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan);
void avdEyeballDestroy(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan);
bool avdEyeballRecalculateVeinsTexture(AVD_Eyeball *eyeball, AVD_Vulkan *vulkan);
bool avdEyeballGetScleraVertexInfo(AVD_Eyeball *eyeball, AVD_UInt32 *indexOffset, AVD_UInt32 *indexCount);
bool avdEyeballGetLensVertexInfo(AVD_Eyeball *eyeball, AVD_UInt32 *indexOffset, AVD_UInt32 *indexCount);

#endif // AVD_EYEBALL_H