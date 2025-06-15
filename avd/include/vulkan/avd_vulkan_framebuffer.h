#ifndef AVD_VULKAN_FRAMEBUFFER_H
#define AVD_VULKAN_FRAMEBUFFER_H

#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_image.h"

typedef struct AVD_VulkanFramebufferAttachment {
    AVD_VulkanImage image;
    VkAttachmentDescription attachmentDescription;
    VkFramebufferAttachmentImageInfo attachmentImageInfo;    

    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
} AVD_VulkanFramebufferAttachment;

typedef struct AVD_VulkanFramebuffer {
    VkFramebuffer framebuffer;
    VkRenderPass renderPass;
    bool hasDepthStencil;

    AVD_List colorAttachments;
    AVD_VulkanFramebufferAttachment depthStencilAttachment;

    uint32_t width;
    uint32_t height;
} AVD_VulkanFramebuffer;


bool avdVulkanFormatIsDepth(VkFormat format);
bool avdVulkanFormatIsStencil(VkFormat format);
bool avdVulkanFormatIsDepthStencil(VkFormat format);
bool avdVulkanFramebufferCreate(AVD_Vulkan *vulkan, AVD_VulkanFramebuffer *framebuffer, int32_t width, int32_t height, bool hasDepthStencil, VkFormat* colorFormats, uint32_t formatCount, VkFormat depthStencilFormat);
void avdVulkanFramebufferDestroy(AVD_Vulkan *vulkan, AVD_VulkanFramebuffer *framebuffer);
AVD_VulkanFramebufferAttachment *avdVulkanFramebufferGetColorAttachment(AVD_VulkanFramebuffer *framebuffer, size_t index);
bool avdVulkanFramebufferGetAttachmentViews(AVD_VulkanFramebuffer *framebuffer, VkImageView *colorAttachmentView, size_t *attachmentCount);

#endif // AVD_VULKAN_FRAMEBUFFER_H