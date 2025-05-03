#include "ps_vulkan.h"

static bool __psVulkanCreateSemaphore(VkDevice device, VkSemaphore *semaphore)
{
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result = vkCreateSemaphore(device, &semaphoreInfo, NULL, semaphore);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create semaphore\n");
        return false;
    }

    return true;
}

static bool __psVulkanCreateFence(VkDevice device, VkFence *fence)
{
    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result = vkCreateFence(device, &fenceInfo, NULL, fence);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to create fence\n");
        return false;
    }

    return true;
}

static bool __psVulkanRendererCreateCommandBuffer(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = gameState->vulkan.graphicsCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = gameState->vulkan.renderer.numInFlightFrames;    

    static VkCommandBuffer commandBuffers[16] = {0};
    VkResult result = vkAllocateCommandBuffers(gameState->vulkan.device, &allocInfo, commandBuffers);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to allocate command buffer\n");
        return false;
    }

    for (uint32_t i = 0; i < gameState->vulkan.renderer.numInFlightFrames; ++i)
    {
        gameState->vulkan.renderer.resources[i].commandBuffer = commandBuffers[i];
    }

    return true;
}

static bool __psVulkanRendererCreateSynchronizationObjects(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    // create the semaphores and fence for each in-flight frame
    for (uint32_t i = 0; i < gameState->vulkan.renderer.numInFlightFrames; ++i)
    {
        if (!__psVulkanCreateSemaphore(gameState->vulkan.device, &gameState->vulkan.renderer.resources[i].imageAvailableSemaphore))
        {
            PS_LOG("Failed to create image available semaphore\n");
            return false;
        }
        if (!__psVulkanCreateSemaphore(gameState->vulkan.device, &gameState->vulkan.renderer.resources[i].renderFinishedSemaphore))
        {
            PS_LOG("Failed to create render finished semaphore\n");
            return false;
        }
        if (!__psVulkanCreateFence(gameState->vulkan.device, &gameState->vulkan.renderer.resources[i].renderFence))
        {
            PS_LOG("Failed to create render fence\n");
            return false;
        }
    }

    return true;
}

static void __psVulkanRendererDestroySynchronizationObjects(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    for (uint32_t i = 0; i < gameState->vulkan.renderer.numInFlightFrames; ++i)
    {
        vkDestroySemaphore(gameState->vulkan.device, gameState->vulkan.renderer.resources[i].imageAvailableSemaphore, NULL);
        vkDestroySemaphore(gameState->vulkan.device, gameState->vulkan.renderer.resources[i].renderFinishedSemaphore, NULL);
        vkDestroyFence(gameState->vulkan.device, gameState->vulkan.renderer.resources[i].renderFence, NULL);
    }
}

static bool __psVulkanRendererCreateRenderResources(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    gameState->vulkan.renderer.numInFlightFrames = PS_MIN(gameState->vulkan.swapchain.imageCount, 2);
    gameState->vulkan.renderer.currentFrameIndex = 0;

    // create the synchronization objects
    if (!__psVulkanRendererCreateSynchronizationObjects(gameState))
    {
        PS_LOG("Failed to create synchronization objects\n");
        return false;
    }

    // create the command buffer
    if (!__psVulkanRendererCreateCommandBuffer(gameState))
    {
        PS_LOG("Failed to create command buffer\n");
        return false;
    }

    return true;
}

static void __psVulkanRendererDestroyRenderResources(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    __psVulkanRendererDestroySynchronizationObjects(gameState);

    for (uint32_t i = 0; i < gameState->vulkan.renderer.numInFlightFrames; ++i)
    {
        vkFreeCommandBuffers(gameState->vulkan.device, gameState->vulkan.graphicsCommandPool, 1, &gameState->vulkan.renderer.resources[i].commandBuffer);
    }
}

static void __psVulkanRendererNextInflightFrame(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    gameState->vulkan.renderer.currentFrameIndex = (gameState->vulkan.renderer.currentFrameIndex + 1) % gameState->vulkan.renderer.numInFlightFrames;
}

static bool __psVulkanRendererHandleSwapchainResult(PS_GameState* gameState, VkResult result) {
    switch (result)
    {
    case VK_SUCCESS:
        return true;
    case VK_SUBOPTIMAL_KHR:
        PS_LOG("Swapchain is suboptimal\n");
        gameState->vulkan.swapchain.swapchainRecreateRequired = true;
        __psVulkanRendererNextInflightFrame(gameState);
        return false;
    case VK_ERROR_OUT_OF_DATE_KHR:
        PS_LOG("Swapchain is out of date\n");
        gameState->vulkan.swapchain.swapchainRecreateRequired = true;
        __psVulkanRendererNextInflightFrame(gameState);
        return false;
    default:
        PS_LOG("Failed to acquire next image from swapchain: %d\n", result);
        exit(1);
    }

    return true;
}

bool psVulkanRendererCreate(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    // create the command buffer and synchronization objects
    if (!__psVulkanRendererCreateRenderResources(gameState))
    {
        PS_LOG("Failed to create render resources\n");
        return false;
    }

    return true;
}

void psVulkanRendererDestroy(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    __psVulkanRendererDestroyRenderResources(gameState);
}

void psVulkanRendererRender(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    if (gameState->vulkan.swapchain.swapchainRecreateRequired)
    {
        PS_LOG("Swapchain recreate required\n");
        psVulkanSwapchainRecreate(gameState);

        __psVulkanRendererDestroyRenderResources(gameState);
        if (!__psVulkanRendererCreateRenderResources(gameState))
        {
            PS_LOG("Failed to create render resources\n");
            exit(1);
        }
        return; // do not render this frame
    }

    uint32_t currentFrameIndex = gameState->vulkan.renderer.currentFrameIndex;
    vkWaitForFences(gameState->vulkan.device, 1, &gameState->vulkan.renderer.resources[currentFrameIndex].renderFence, VK_TRUE, UINT64_MAX);
    vkResetFences(gameState->vulkan.device, 1, &gameState->vulkan.renderer.resources[currentFrameIndex].renderFence);

    uint32_t imageIndex = 0;
    VkResult result = psVulkanSwapchainAcquireNextImage(gameState, &imageIndex, gameState->vulkan.renderer.resources[currentFrameIndex].imageAvailableSemaphore);
    if (!__psVulkanRendererHandleSwapchainResult(gameState, result))
    {
        PS_LOG("Failed to acquire next image from swapchain\n");
        return;
    }
    
    VkCommandBuffer commandBuffer = gameState->vulkan.renderer.resources[currentFrameIndex].commandBuffer;
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = NULL;
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to begin command buffer: %d\n", result);
        vkResetFences(gameState->vulkan.device, 1, &gameState->vulkan.renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(gameState);
        return; // do not render this frame
    }

    if(!psVulkanRendererRenderPresentationPass(gameState, imageIndex)) {
        PS_LOG("Failed to render presentation pass\n");
        vkResetFences(gameState->vulkan.device, 1, &gameState->vulkan.renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(gameState);
        return; // do not render this frame
    }

    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to end command buffer: %d\n", result);
        vkResetFences(gameState->vulkan.device, 1, &gameState->vulkan.renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(gameState);
        return; // do not render this frame
    }


    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &gameState->vulkan.renderer.resources[currentFrameIndex].imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &gameState->vulkan.renderer.resources[currentFrameIndex].renderFinishedSemaphore;

    result = vkQueueSubmit(gameState->vulkan.graphicsQueue, 1, &submitInfo, gameState->vulkan.renderer.resources[currentFrameIndex].renderFence);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to submit command buffer: %d\n", result);
        vkResetFences(gameState->vulkan.device, 1, &gameState->vulkan.renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(gameState);
        return; // do not render this frame
    }

    result = psVulkanSwapchainPresent(gameState, imageIndex, gameState->vulkan.renderer.resources[currentFrameIndex].renderFinishedSemaphore);
    if (!__psVulkanRendererHandleSwapchainResult(gameState, result))
    {
        PS_LOG("Failed to present swapchain image\n");
        return; // do not render this frame
    }

    __psVulkanRendererNextInflightFrame(gameState);
}

bool psVulkanRendererRenderPresentationPass(PS_GameState *gameState, uint32_t imageIndex)
{
    PS_ASSERT(gameState != NULL);
    uint32_t currentFrameIndex = gameState->vulkan.renderer.currentFrameIndex;
    VkCommandBuffer commandBuffer = gameState->vulkan.renderer.resources[currentFrameIndex].commandBuffer;

    static VkClearValue clearColor[2] = {0};
    clearColor[0].color.float32[0] = 0.1f;
    clearColor[0].color.float32[1] = 0.1f;
    clearColor[0].color.float32[2] = 0.1f;
    clearColor[0].color.float32[3] = 1.0f;

    clearColor[1].depthStencil.depth = 1.0f;
    clearColor[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = gameState->vulkan.swapchain.renderPass;
    renderPassInfo.framebuffer = gameState->vulkan.swapchain.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = gameState->vulkan.swapchain.extent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // TODO: Add rendering commands here

    vkCmdEndRenderPass(commandBuffer);

    return true;
}