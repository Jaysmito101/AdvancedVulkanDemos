#include "vulkan/avd_vulkan_swapchain.h"

static bool __avdVulkanSwapchainChooseSurfaceformat(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(swapchain != NULL);

    uint32_t formatCount                         = 0;
    static VkSurfaceFormatKHR surfaceFormats[64] = {0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physicalDevice, swapchain->surface, &formatCount, NULL);
    AVD_CHECK_MSG(formatCount > 0, "No Vulkan-compatible surface formats found\n");
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physicalDevice, swapchain->surface, &formatCount, surfaceFormats);

    for (uint32_t i = 0; i < formatCount; ++i) {
        if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain->surfaceFormat = surfaceFormats[i];
            return true;
        }
    }

    AVD_LOG("Preferred surface format not found, using the first available format\n");
    swapchain->surfaceFormat = surfaceFormats[0];

    return true;
}

static bool __avdVulkanSwapchainChoosePresentMode(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(swapchain != NULL);

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physicalDevice, swapchain->surface, &presentModeCount, NULL);
    AVD_CHECK_MSG(presentModeCount > 0, "No Vulkan-compatible present modes found\n");

    static VkPresentModeKHR presentModes[64] = {0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physicalDevice, swapchain->surface, &presentModeCount, presentModes);

    for (uint32_t i = 0; i < presentModeCount; ++i) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchain->presentMode = presentModes[i];
            return true;
        }
    }

    AVD_LOG("Preferred present mode not found, using FIFO mode\n");
    swapchain->presentMode = VK_PRESENT_MODE_FIFO_KHR;

    return true;
}

static bool __avdVulkanSwapchainChooseExtent(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan, uint32_t fallBackWidth, uint32_t fallBackHeight)
{
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(fallBackWidth > 0);
    AVD_ASSERT(fallBackHeight > 0);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->physicalDevice, swapchain->surface, &capabilities);

    if (capabilities.currentExtent.width != UINT32_MAX) {
        swapchain->extent = capabilities.currentExtent;
        return true;
    }

    uint32_t minWidth = capabilities.minImageExtent.width;
    uint32_t maxWidth = capabilities.maxImageExtent.width;

    uint32_t minHeight = capabilities.minImageExtent.height;
    uint32_t maxHeight = capabilities.maxImageExtent.height;

    uint32_t width  = AVD_CLAMP(fallBackWidth, minWidth, maxWidth);
    uint32_t height = AVD_CLAMP(fallBackHeight, minHeight, maxHeight);

    swapchain->extent.width  = width;
    swapchain->extent.height = height;

    return true;
}

static bool __avdVulkanSwapchainChooseImageCount(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(vulkan != NULL);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->physicalDevice, swapchain->surface, &capabilities);

    swapchain->imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && swapchain->imageCount > capabilities.maxImageCount) {
        swapchain->imageCount = capabilities.maxImageCount;
    }

    return true;
}

static bool __avdVulkanSwapchainChooseParameters(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan, uint32_t windowWidth, uint32_t windowHeight)
{
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(windowWidth > 0);
    AVD_ASSERT(windowHeight > 0);

    AVD_CHECK(__avdVulkanSwapchainChooseSurfaceformat(swapchain, vulkan));
    AVD_CHECK(__avdVulkanSwapchainChoosePresentMode(swapchain, vulkan));
    AVD_CHECK(__avdVulkanSwapchainChooseExtent(swapchain, vulkan, windowWidth, windowHeight));
    AVD_CHECK(__avdVulkanSwapchainChooseImageCount(swapchain, vulkan));
    return true;
}

static bool __avdVulkanSwapchainQueryImages(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(swapchain != NULL);

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(vulkan->device, swapchain->swapchain, &imageCount, NULL);
    AVD_CHECK_MSG(imageCount > 0, "No Vulkan-compatible swapchain images found\n");
    AVD_CHECK_MSG(imageCount == swapchain->imageCount, "Swapchain image count mismatch\n");
    vkGetSwapchainImagesKHR(vulkan->device, swapchain->swapchain, &imageCount, swapchain->images);

    return true;
}

static bool __avdVulkanSwapchainCreateImageViews(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(vulkan != NULL);

    for (uint32_t i = 0; i < swapchain->imageCount; ++i) {
        VkImageViewCreateInfo viewInfo           = {0};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = swapchain->images[i];
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = swapchain->surfaceFormat.format;
        viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        VkResult result = vkCreateImageView(vulkan->device, &viewInfo, NULL, &swapchain->imageViews[i]);
        AVD_CHECK_VK_RESULT(result, "Failed to create image view for image %d\n", i);
    }
    return true;
}

static bool __avdVulkanSwapchainCreateFramebuffers(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(swapchain != NULL);

    for (uint32_t i = 0; i < swapchain->imageCount; ++i) {
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass              = swapchain->renderPass;
        framebufferInfo.attachmentCount         = 1;
        framebufferInfo.pAttachments            = &swapchain->imageViews[i];
        framebufferInfo.width                   = swapchain->extent.width;
        framebufferInfo.height                  = swapchain->extent.height;
        framebufferInfo.layers                  = 1;

        VkResult result = vkCreateFramebuffer(vulkan->device, &framebufferInfo, NULL, &swapchain->framebuffers[i]);
        AVD_CHECK_VK_RESULT(result, "Failed to create framebuffer for image view %d\n", i);
    }
    return true;
}

static bool __avdVulkanSwapchainKHRCreate(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan, VkSwapchainKHR oldSwapchain)
{
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(vulkan != NULL);

    uint32_t queueFamilyIndices[1] = {vulkan->graphicsQueueFamilyIndex};

    VkSwapchainCreateInfoKHR swapchainInfo = {0};
    swapchainInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface                  = swapchain->surface;
    swapchainInfo.minImageCount            = swapchain->imageCount;
    swapchainInfo.imageFormat              = swapchain->surfaceFormat.format;
    swapchainInfo.imageColorSpace          = swapchain->surfaceFormat.colorSpace;
    swapchainInfo.imageExtent              = swapchain->extent;
    swapchainInfo.imageArrayLayers         = 1;
    swapchainInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchainInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount    = 1;
    swapchainInfo.pQueueFamilyIndices      = queueFamilyIndices;
    swapchainInfo.preTransform             = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode              = swapchain->presentMode;
    swapchainInfo.clipped                  = VK_TRUE;
    swapchainInfo.oldSwapchain             = oldSwapchain;

    VkResult result = vkCreateSwapchainKHR(vulkan->device, &swapchainInfo, NULL, &swapchain->swapchain);
    AVD_CHECK_VK_RESULT(result, "Failed to create swapchain\n");

    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkan->device, oldSwapchain, NULL);
    }
    AVD_LOG("Swapchain created successfully\n");

    return true;
}

static bool __avdVulkanSwapchainCreateRenderPass(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(vulkan != NULL);

    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format                  = swapchain->surfaceFormat.format;
    colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment            = 0;
    colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass          = 0;
    dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask       = 0;
    dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = 1;
    renderPassInfo.pAttachments           = &colorAttachment;
    renderPassInfo.subpassCount           = 1;
    renderPassInfo.pSubpasses             = &subpass;
    renderPassInfo.dependencyCount        = 1;
    renderPassInfo.pDependencies          = &dependency;

    VkResult result = vkCreateRenderPass(vulkan->device, &renderPassInfo, NULL, &swapchain->renderPass);
    AVD_CHECK_VK_RESULT(result, "Failed to create render pass\n");

    return true;
}

bool avdVulkanSwapchainCreate(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan, VkSurfaceKHR surface, AVD_Window *window)
{
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(surface != VK_NULL_HANDLE);
    AVD_ASSERT(window != NULL);

    swapchain->surface = surface;

    AVD_CHECK(__avdVulkanSwapchainChooseParameters(swapchain, vulkan, window->width, window->height));
    AVD_CHECK(__avdVulkanSwapchainKHRCreate(swapchain, vulkan, VK_NULL_HANDLE));
    AVD_CHECK(__avdVulkanSwapchainCreateRenderPass(swapchain, vulkan));
    AVD_CHECK(__avdVulkanSwapchainQueryImages(swapchain, vulkan));
    AVD_CHECK(__avdVulkanSwapchainCreateImageViews(swapchain, vulkan));
    AVD_CHECK(__avdVulkanSwapchainCreateFramebuffers(swapchain, vulkan));

    swapchain->swapchainReady            = true;
    swapchain->swapchainRecreateRequired = false;

    return true;
}

bool avdVulkanSwapchainRecreate(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan, AVD_Window *window)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(swapchain != NULL);

    vkDeviceWaitIdle(vulkan->device);

    if (swapchain->swapchainReady) {
        for (uint32_t i = 0; i < swapchain->imageCount; ++i) {
            vkDestroyImageView(vulkan->device, swapchain->imageViews[i], NULL);
            vkDestroyFramebuffer(vulkan->device, swapchain->framebuffers[i], NULL);
        }
    }

    AVD_CHECK(__avdVulkanSwapchainChooseParameters(swapchain, vulkan, window->width, window->height));
    AVD_CHECK(__avdVulkanSwapchainKHRCreate(swapchain, vulkan, swapchain->swapchain));
    AVD_CHECK(__avdVulkanSwapchainQueryImages(swapchain, vulkan));
    AVD_CHECK(__avdVulkanSwapchainCreateImageViews(swapchain, vulkan));
    AVD_CHECK(__avdVulkanSwapchainCreateFramebuffers(swapchain, vulkan));

    swapchain->swapchainReady            = true;
    swapchain->swapchainRecreateRequired = false;

    vkDeviceWaitIdle(vulkan->device);

    return true;
}

void avdVulkanSwapchainDestroy(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(vulkan != NULL);

    if (!swapchain->swapchainReady) {
        return;
    }

    vkDeviceWaitIdle(vulkan->device);

    vkDestroyRenderPass(vulkan->device, swapchain->renderPass, NULL);
    swapchain->renderPass = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < swapchain->imageCount; ++i) {
        vkDestroyImageView(vulkan->device, swapchain->imageViews[i], NULL);
        vkDestroyFramebuffer(vulkan->device, swapchain->framebuffers[i], NULL);
    }

    vkDestroySwapchainKHR(vulkan->device, swapchain->swapchain, NULL);
    swapchain->swapchainReady = false;
}

VkResult avdVulkanSwapchainAcquireNextImage(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan, uint32_t *imageIndex, VkSemaphore semaphore, VkFence fence)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(imageIndex != NULL);
    AVD_ASSERT(swapchain != NULL);

    VkResult result = vkAcquireNextImageKHR(vulkan->device, swapchain->swapchain, UINT64_MAX, semaphore, fence, imageIndex);
    if (result != VK_SUCCESS) {
        AVD_LOG("Failed to acquire next image from swapchain\n");
        return result;
    }

    return VK_SUCCESS;
}

VkResult avdVulkanSwapchainPresent(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan, uint32_t imageIndex, VkSemaphore waitSemaphore)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(swapchain != NULL);

    VkPresentInfoKHR presentInfo   = {0};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &waitSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain->swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    VkResult result = vkQueuePresentKHR(vulkan->graphicsQueue, &presentInfo);
    if (result != VK_SUCCESS) {
        AVD_LOG("Failed to present image to swapchain\n");
        return result;
    }

    return VK_SUCCESS;
}

bool avdVulkanSwapchainRecreateIfNeeded(AVD_VulkanSwapchain *swapchain, AVD_Vulkan *vulkan, AVD_Window *window)
{
    AVD_ASSERT(swapchain != NULL);

    if (swapchain->swapchainRecreateRequired) {
        AVD_LOG("Swapchain recreate required\n");
        avdVulkanSwapchainRecreate(swapchain, vulkan, window);
        return true;
    }
    return false;
}