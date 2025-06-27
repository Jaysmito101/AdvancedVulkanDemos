#ifndef AVD_VULKAN_PRESENTATION_H
#define AVD_VULKAN_PRESENTATION_H

#include "font/avd_font_renderer.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_image.h"

struct AVD_SceneManager;

typedef struct AVD_VulkanPresentation {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    AVD_RenderableText loadingText;
    AVD_RenderableText loadingStatusText;
    AVD_FontRenderer presentationFontRenderer;
    VkDescriptorSetLayout descriptorSetLayout;
} AVD_VulkanPresentation;

bool avdVulkanPresentationInit(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain, AVD_FontManager *fontManager);
void avdVulkanPresentationDestroy(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan);
bool avdVulkanPresentationRender(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanRenderer *renderer, AVD_VulkanSwapchain *swapchain, struct AVD_SceneManager *sceneManager, uint32_t imageIndex);

#endif // AVD_VULKAN_PRESENTATION_H