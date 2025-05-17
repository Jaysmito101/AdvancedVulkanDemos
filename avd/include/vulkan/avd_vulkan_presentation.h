#ifndef AVD_VULKAN_PRESENTATION_H
#define AVD_VULKAN_PRESENTATION_H

#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_image.h"


typedef struct AVD_VulkanPresentation {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorSetLayout iconDescriptorSetLayout;
    VkDescriptorSet iconDescriptorSet;
    AVD_VulkanImage iconImage;

} AVD_VulkanPresentation;

bool avdVulkanPresentationInit(AVD_VulkanPresentation* presentation, AVD_Vulkan *vulkan, AVD_VulkanSwapchain* swapchain);
void avdVulkanPresentationDestroy(AVD_VulkanPresentation* presentation, AVD_Vulkan *vulkan);
bool avdVulkanPresentationRender(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanRenderer *renderer, AVD_VulkanSwapchain *swapchain, uint32_t imageIndex);

#endif // AVD_VULKAN_PRESENTATION_H