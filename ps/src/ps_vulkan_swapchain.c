#include "ps_vulkan.h"

static bool __psVulkanSwapchainChooseSurfaceformat(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);

    uint32_t formatCount = 0;
    static VkSurfaceFormatKHR surfaceFormats[64] = {0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physicalDevice, vulkan->swapchain.surface, &formatCount, NULL);
    PS_CHECK_MSG(formatCount > 0, "No Vulkan-compatible surface formats found\n");
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physicalDevice, vulkan->swapchain.surface, &formatCount, surfaceFormats);

    for (uint32_t i = 0; i < formatCount; ++i) {
        if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            vulkan->swapchain.surfaceFormat = surfaceFormats[i];
            return true;
        }
    }

    PS_LOG("Preferred surface format not found, using the first available format\n");
    vulkan->swapchain.surfaceFormat = surfaceFormats[0];

    return true;
}

static bool __psVulkanSwapchainChoosePresentMode(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physicalDevice, vulkan->swapchain.surface, &presentModeCount, NULL);
    PS_CHECK_MSG(presentModeCount > 0, "No Vulkan-compatible present modes found\n");
    
    static VkPresentModeKHR presentModes[64] = {0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physicalDevice, vulkan->swapchain.surface, &presentModeCount, presentModes);
    
    for (uint32_t i = 0; i < presentModeCount; ++i) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            vulkan->swapchain.presentMode = presentModes[i];
            return true;
        }
    }

    PS_LOG("Preferred present mode not found, using FIFO mode\n");
    vulkan->swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    return true;
}

static bool __psVulkanSwapchainChooseExtent(PS_Vulkan *vulkan, PS_Window* window) {
    PS_ASSERT(vulkan != NULL);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->physicalDevice, vulkan->swapchain.surface, &capabilities);

    if (capabilities.currentExtent.width != UINT32_MAX) {
        vulkan->swapchain.extent = capabilities.currentExtent;
        return true;
    }

    uint32_t minWidth = capabilities.minImageExtent.width;
    uint32_t maxWidth = capabilities.maxImageExtent.width;

    uint32_t minHeight = capabilities.minImageExtent.height;
    uint32_t maxHeight = capabilities.maxImageExtent.height;

    uint32_t width = PS_CLAMP((uint32_t)window->width, minWidth, maxWidth);
    uint32_t height = PS_CLAMP((uint32_t)window->height, minHeight, maxHeight);

    vulkan->swapchain.extent.width = width;
    vulkan->swapchain.extent.height = height;

    return true;
}

static bool __psVulkanSwapchainChooseImageCount(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->physicalDevice, vulkan->swapchain.surface, &capabilities);

    vulkan->swapchain.imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && vulkan->swapchain.imageCount > capabilities.maxImageCount) {
        vulkan->swapchain.imageCount = capabilities.maxImageCount;
    }

    return true;
}

static bool __psVulkanSwapchainChooseParameters(PS_Vulkan *vulkan, PS_Window* window) {
    PS_ASSERT(vulkan != NULL);
    PS_CHECK(__psVulkanSwapchainChooseSurfaceformat(vulkan));
    PS_CHECK(__psVulkanSwapchainChoosePresentMode(vulkan));
    PS_CHECK(__psVulkanSwapchainChooseExtent(vulkan, window));
    PS_CHECK(__psVulkanSwapchainChooseImageCount(vulkan));
    return true;
}

static bool __psVulkanSwapchainQueryImages(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(vulkan->device, vulkan->swapchain.swapchain, &imageCount, NULL);
    PS_CHECK_MSG(imageCount > 0, "No Vulkan-compatible swapchain images found\n");
    PS_CHECK_MSG(imageCount == vulkan->swapchain.imageCount, "Swapchain image count mismatch\n");
    vkGetSwapchainImagesKHR(vulkan->device, vulkan->swapchain.swapchain, &imageCount, vulkan->swapchain.images);

    return true;
}

static bool __psVulkanSwapchainCreateImageViews(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);

    for (uint32_t i = 0; i < vulkan->swapchain.imageCount; ++i) {
        VkImageViewCreateInfo viewInfo = {0};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = vulkan->swapchain.images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = vulkan->swapchain.surfaceFormat.format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(vulkan->device, &viewInfo, NULL, &vulkan->swapchain.imageViews[i]);
        PS_CHECK_VK_RESULT(result, "Failed to create image view for image %d\n", i);
    }
    return true;
}

static bool __psVulkanSwapchainCreateFramebuffers(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);

    for (uint32_t i = 0; i < vulkan->swapchain.imageCount; ++i) {
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vulkan->swapchain.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vulkan->swapchain.imageViews[i];
        framebufferInfo.width = vulkan->swapchain.extent.width;
        framebufferInfo.height = vulkan->swapchain.extent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(vulkan->device, &framebufferInfo, NULL, &vulkan->swapchain.framebuffers[i]);
        PS_CHECK_VK_RESULT(result, "Failed to create framebuffer for image view %d\n", i);
    }
    return true;
}

static bool __psVulkanSwapchainKHRCreate(PS_Vulkan* vulkan, VkSwapchainKHR oldSwapchain) {
    PS_ASSERT(vulkan != NULL);

    uint32_t queueFamilyIndices[1] = {vulkan->graphicsQueueFamilyIndex};
    
    VkSwapchainCreateInfoKHR swapchainInfo = {0};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = vulkan->swapchain.surface;
    swapchainInfo.minImageCount = vulkan->swapchain.imageCount;
    swapchainInfo.imageFormat = vulkan->swapchain.surfaceFormat.format;
    swapchainInfo.imageColorSpace = vulkan->swapchain.surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = vulkan->swapchain.extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 1;
    swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = vulkan->swapchain.presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = oldSwapchain;

    VkResult result = vkCreateSwapchainKHR(vulkan->device, &swapchainInfo, NULL, &vulkan->swapchain.swapchain);
    PS_CHECK_VK_RESULT(result, "Failed to create swapchain\n");

    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkan->device, oldSwapchain, NULL);
    }
    PS_LOG("Swapchain created successfully\n");

    return true;
}

static bool __psVulkanSwapchainCreateRenderPass(PS_Vulkan* vulkan) {
    PS_ASSERT(vulkan != NULL);

    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = vulkan->swapchain.surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;


    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(vulkan->device, &renderPassInfo, NULL, &vulkan->swapchain.renderPass);
    PS_CHECK_VK_RESULT(result, "Failed to create render pass\n");

    return true;
}

bool psVulkanSwapchainCreate(PS_Vulkan *vulkan, PS_Window* window) {
    PS_ASSERT(vulkan != NULL);

    if (vulkan->swapchain.swapchainReady) {
        PS_LOG("Swapchain already created\n");
        return true;
    }

    PS_CHECK(__psVulkanSwapchainChooseParameters(vulkan, window));
    PS_CHECK(__psVulkanSwapchainKHRCreate(vulkan, VK_NULL_HANDLE));
    PS_CHECK(__psVulkanSwapchainCreateRenderPass(vulkan));
    PS_CHECK(__psVulkanSwapchainQueryImages(vulkan));
    PS_CHECK(__psVulkanSwapchainCreateImageViews(vulkan));
    PS_CHECK(__psVulkanSwapchainCreateFramebuffers(vulkan));

    vulkan->swapchain.swapchainReady = true;
    vulkan->swapchain.swapchainRecreateRequired = false;
    return true;
}

bool psVulkanSwapchainRecreate(PS_Vulkan *vulkan, PS_Window* window) {
    PS_ASSERT(vulkan != NULL);
    PS_ASSERT(window != NULL);

    vkDeviceWaitIdle(vulkan->device);

    if (vulkan->swapchain.swapchainReady) {
        for (uint32_t i = 0; i < vulkan->swapchain.imageCount; ++i) {
            vkDestroyImageView(vulkan->device, vulkan->swapchain.imageViews[i], NULL);
            vkDestroyFramebuffer(vulkan->device, vulkan->swapchain.framebuffers[i], NULL);
        }
    }

    PS_CHECK(__psVulkanSwapchainChooseParameters(vulkan, window));
    PS_CHECK(__psVulkanSwapchainKHRCreate(vulkan, vulkan->swapchain.swapchain));
    PS_CHECK(__psVulkanSwapchainQueryImages(vulkan));
    PS_CHECK(__psVulkanSwapchainCreateImageViews(vulkan));
    PS_CHECK(__psVulkanSwapchainCreateFramebuffers(vulkan));

    vulkan->swapchain.swapchainReady = true;
    vulkan->swapchain.swapchainRecreateRequired = false;

    vkDeviceWaitIdle(vulkan->device);

    return true;
}

void psVulkanSwapchainDestroy(PS_Vulkan *vulkan) {
    PS_ASSERT(vulkan != NULL);

    if (!vulkan->swapchain.swapchainReady) {
        return;
    }

    vkDeviceWaitIdle(vulkan->device);

    vkDestroyRenderPass(vulkan->device, vulkan->swapchain.renderPass, NULL);
    vulkan->swapchain.renderPass = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < vulkan->swapchain.imageCount; ++i) {
        vkDestroyImageView(vulkan->device, vulkan->swapchain.imageViews[i], NULL);
        vkDestroyFramebuffer(vulkan->device, vulkan->swapchain.framebuffers[i], NULL);
    }

    vkDestroySwapchainKHR(vulkan->device, vulkan->swapchain.swapchain, NULL);
    vulkan->swapchain.swapchainReady = false;
}

VkResult psVulkanSwapchainAcquireNextImage(PS_Vulkan *vulkan, uint32_t *imageIndex, VkSemaphore semaphore) {
    PS_ASSERT(vulkan != NULL);
    PS_ASSERT(imageIndex != NULL);

    VkResult result = vkAcquireNextImageKHR(vulkan->device, vulkan->swapchain.swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, imageIndex);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to acquire next image from swapchain\n");
        return result;
    }

    return VK_SUCCESS;
}

VkResult psVulkanSwapchainPresent(PS_Vulkan *vulkan, uint32_t imageIndex, VkSemaphore waitSemaphore) {
    PS_ASSERT(vulkan != NULL);

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkan->swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult result = vkQueuePresentKHR(vulkan->graphicsQueue, &presentInfo);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to present image to swapchain\n");
        return result;
    }

    return VK_SUCCESS;
}