#include "ps_vulkan.h"

static bool __psVulkanSwapchainChooseSurfaceformat(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    uint32_t formatCount = 0;
    static VkSurfaceFormatKHR surfaceFormats[64] = {0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(gameState->vulkan.physicalDevice, gameState->vulkan.swapchain.surface, &formatCount, NULL);
    if (formatCount == 0) {
        PS_LOG("No Vulkan-compatible surface formats found\n");
        return false;
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(gameState->vulkan.physicalDevice, gameState->vulkan.swapchain.surface, &formatCount, surfaceFormats);
    
    for (uint32_t i = 0; i < formatCount; ++i) {
        if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            gameState->vulkan.swapchain.surfaceFormat = surfaceFormats[i];
            return true;
        }
    }

    PS_LOG("Preferred surface format not found, using the first available format\n");
    gameState->vulkan.swapchain.surfaceFormat = surfaceFormats[0];

    return true;
}

static bool __psVulkanSwapchainChoosePresentMode(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gameState->vulkan.physicalDevice, gameState->vulkan.swapchain.surface, &presentModeCount, NULL);
    if (presentModeCount == 0) {
        PS_LOG("No Vulkan-compatible present modes found\n");
        return false;
    }
    
    static VkPresentModeKHR presentModes[64] = {0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(gameState->vulkan.physicalDevice, gameState->vulkan.swapchain.surface, &presentModeCount, presentModes);
    
    for (uint32_t i = 0; i < presentModeCount; ++i) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            gameState->vulkan.swapchain.presentMode = presentModes[i];
            return true;
        }
    }

    PS_LOG("Preferred present mode not found, using FIFO mode\n");
    gameState->vulkan.swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    return true;
}

static bool __psVulkanSwapchainChooseExtent(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gameState->vulkan.physicalDevice, gameState->vulkan.swapchain.surface, &capabilities);

    if (capabilities.currentExtent.width != UINT32_MAX) {
        gameState->vulkan.swapchain.extent = capabilities.currentExtent;
        return true;
    }

    uint32_t minWidth = capabilities.minImageExtent.width;
    uint32_t maxWidth = capabilities.maxImageExtent.width;

    uint32_t minHeight = capabilities.minImageExtent.height;
    uint32_t maxHeight = capabilities.maxImageExtent.height;

    uint32_t width = PS_CLAMP((uint32_t)gameState->window.width, minWidth, maxWidth);
    uint32_t height = PS_CLAMP((uint32_t)gameState->window.height, minHeight, maxHeight);

    gameState->vulkan.swapchain.extent.width = width;
    gameState->vulkan.swapchain.extent.height = height;

    return true;
}

static bool __psVulkanSwapchainChooseImageCount(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gameState->vulkan.physicalDevice, gameState->vulkan.swapchain.surface, &capabilities);

    gameState->vulkan.swapchain.imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && gameState->vulkan.swapchain.imageCount > capabilities.maxImageCount) {
        gameState->vulkan.swapchain.imageCount = capabilities.maxImageCount;
    }

    return true;
}

static bool __psVulkanSwapchainChooseParameters(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    if (!__psVulkanSwapchainChooseSurfaceformat(gameState)) {
        PS_LOG("Failed to choose surface format\n");
        return false;
    }

    if (!__psVulkanSwapchainChoosePresentMode(gameState)) {
        PS_LOG("Failed to choose present mode\n");
        return false;
    }

    if (!__psVulkanSwapchainChooseExtent(gameState)) {
        PS_LOG("Failed to choose extent\n");
        return false;
    }

    if (!__psVulkanSwapchainChooseImageCount(gameState)) {
        PS_LOG("Failed to choose image count\n");
        return false;
    }

    return true;
}

static bool __psVulkanSwapchainQueryImages(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(gameState->vulkan.device, gameState->vulkan.swapchain.swapchain, &imageCount, NULL);
    if (imageCount == 0) {
        PS_LOG("No Vulkan-compatible swapchain images found\n");
        return false;
    }
    if (imageCount != gameState->vulkan.swapchain.imageCount) {
        PS_LOG("Swapchain image count mismatch\n");
        return false;
    }

    vkGetSwapchainImagesKHR(gameState->vulkan.device, gameState->vulkan.swapchain.swapchain, &imageCount, gameState->vulkan.swapchain.images);
    
    return true;
}

static bool __psVulkanSwapchainCreateImageViews(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    for (uint32_t i = 0; i < gameState->vulkan.swapchain.imageCount; ++i) {
        VkImageViewCreateInfo viewInfo = {0};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = gameState->vulkan.swapchain.images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = gameState->vulkan.swapchain.surfaceFormat.format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(gameState->vulkan.device, &viewInfo, NULL, &gameState->vulkan.swapchain.imageViews[i]);
        if (result != VK_SUCCESS) {
            PS_LOG("Failed to create image views\n");
            return false;
        }
    }
    return true;
}

static bool __psVulkanSwapchainCreateFramebuffers(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    for (uint32_t i = 0; i < gameState->vulkan.swapchain.imageCount; ++i) {
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = gameState->vulkan.swapchain.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &gameState->vulkan.swapchain.imageViews[i];
        framebufferInfo.width = gameState->vulkan.swapchain.extent.width;
        framebufferInfo.height = gameState->vulkan.swapchain.extent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(gameState->vulkan.device, &framebufferInfo, NULL, &gameState->vulkan.swapchain.framebuffers[i]);
        if (result != VK_SUCCESS) {
            PS_LOG("Failed to create framebuffer for image view %d\n", i);
            return false;
        }
    }
    return true;
}

static bool __psVulkanSwapchainKHRCreate(PS_GameState* gameState, VkSwapchainKHR oldSwapchain) {
    uint32_t queueFamilyIndices[1] = {gameState->vulkan.graphicsQueueFamilyIndex};
    
    VkSwapchainCreateInfoKHR swapchainInfo = {0};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = gameState->vulkan.swapchain.surface;
    swapchainInfo.minImageCount = gameState->vulkan.swapchain.imageCount;
    swapchainInfo.imageFormat = gameState->vulkan.swapchain.surfaceFormat.format;
    swapchainInfo.imageColorSpace = gameState->vulkan.swapchain.surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = gameState->vulkan.swapchain.extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 1;
    swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = gameState->vulkan.swapchain.presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = oldSwapchain;
    
    VkResult result = vkCreateSwapchainKHR(gameState->vulkan.device, &swapchainInfo, NULL, &gameState->vulkan.swapchain.swapchain);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create swapchain\n");
        return false;
    }

    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(gameState->vulkan.device, oldSwapchain, NULL);
    }
    PS_LOG("Swapchain created successfully\n");

    return true;
}

static bool __psVulkanSwapchainCreateRenderPass(PS_GameState* gameState) {
    PS_ASSERT(gameState != NULL);

    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = gameState->vulkan.swapchain.surfaceFormat.format;
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

    VkResult result = vkCreateRenderPass(gameState->vulkan.device, &renderPassInfo, NULL, &gameState->vulkan.swapchain.renderPass);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create render pass\n");
        return false;
    }

    return true;
}

bool psVulkanSwapchainCreate(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    if (gameState->vulkan.swapchain.swapchainReady) {
        PS_LOG("Swapchain already created\n");
        return true;
    }

    if (!__psVulkanSwapchainChooseParameters(gameState)) {
        PS_LOG("Failed to choose swapchain parameters\n");
        return false;
    }

    if (!__psVulkanSwapchainKHRCreate(gameState, VK_NULL_HANDLE)) {
        PS_LOG("Failed to create swapchain\n");
        return false;
    }

    if (!__psVulkanSwapchainCreateRenderPass(gameState)) {
        PS_LOG("Failed to create render pass\n");
        return false;
    }

    if (!__psVulkanSwapchainQueryImages(gameState)) {
        PS_LOG("Failed to query swapchain images\n");
        return false;
    }

    if (!__psVulkanSwapchainCreateImageViews(gameState)) {
        PS_LOG("Failed to create swapchain image views\n");
        return false;
    }

    if (!__psVulkanSwapchainCreateFramebuffers(gameState)) {
        PS_LOG("Failed to create swapchain framebuffers\n");
        return false;
    }

    gameState->vulkan.swapchain.swapchainReady = true;
    gameState->vulkan.swapchain.swapchainRecreateRequired = false;
    return true;
}

bool psVulkanSwapchainRecreate(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    
    vkDeviceWaitIdle(gameState->vulkan.device);
    
    if (gameState->vulkan.swapchain.swapchainReady) {        
        for (uint32_t i = 0; i < gameState->vulkan.swapchain.imageCount; ++i) {
            vkDestroyImageView(gameState->vulkan.device, gameState->vulkan.swapchain.imageViews[i], NULL);
            vkDestroyFramebuffer(gameState->vulkan.device, gameState->vulkan.swapchain.framebuffers[i], NULL);
        }
    }

    if (!__psVulkanSwapchainChooseParameters(gameState)) {
        PS_LOG("Failed to choose swapchain parameters\n");
        return false;
    }    

    if (!__psVulkanSwapchainKHRCreate(gameState, gameState->vulkan.swapchain.swapchain)) {
        PS_LOG("Failed to create swapchain\n");
        return false;
    }

    if (!__psVulkanSwapchainQueryImages(gameState)) {
        PS_LOG("Failed to query swapchain images\n");
        return false;
    }

    if (!__psVulkanSwapchainCreateImageViews(gameState)) {
        PS_LOG("Failed to create swapchain image views\n");
        return false;
    }

    if (!__psVulkanSwapchainCreateFramebuffers(gameState)) {
        PS_LOG("Failed to create swapchain framebuffers\n");
        return false;
    }

    
    gameState->vulkan.swapchain.swapchainReady = true;
    gameState->vulkan.swapchain.swapchainRecreateRequired = false;
    
    vkDeviceWaitIdle(gameState->vulkan.device);

    return true;
}

void psVulkanSwapchainDestroy(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);
    
    if (!gameState->vulkan.swapchain.swapchainReady) {
        return;
    }
    
    vkDeviceWaitIdle(gameState->vulkan.device);

    vkDestroyRenderPass(gameState->vulkan.device, gameState->vulkan.swapchain.renderPass, NULL);
    gameState->vulkan.swapchain.renderPass = VK_NULL_HANDLE;
    
    for (uint32_t i = 0; i < gameState->vulkan.swapchain.imageCount; ++i) {
        vkDestroyImageView(gameState->vulkan.device, gameState->vulkan.swapchain.imageViews[i], NULL);
        vkDestroyFramebuffer(gameState->vulkan.device, gameState->vulkan.swapchain.framebuffers[i], NULL);
    }
    
    vkDestroySwapchainKHR(gameState->vulkan.device, gameState->vulkan.swapchain.swapchain, NULL);
    gameState->vulkan.swapchain.swapchainReady = false;
}

VkResult psVulkanSwapchainAcquireNextImage(PS_GameState *gameState, uint32_t *imageIndex, VkSemaphore semaphore) {
    PS_ASSERT(gameState != NULL);
    PS_ASSERT(imageIndex != NULL);

    VkResult result = vkAcquireNextImageKHR(gameState->vulkan.device, gameState->vulkan.swapchain.swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, imageIndex);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to acquire next image from swapchain\n");
        return result;
    }

    return VK_SUCCESS;
}

VkResult psVulkanSwapchainPresent(PS_GameState *gameState, uint32_t imageIndex, VkSemaphore waitSemaphore) {
    PS_ASSERT(gameState != NULL);

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &gameState->vulkan.swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult result = vkQueuePresentKHR(gameState->vulkan.graphicsQueue, &presentInfo);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to present image to swapchain\n");
        return result;
    }

    return VK_SUCCESS;
}