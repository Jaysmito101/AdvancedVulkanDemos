#include "ps_vulkan.h"
#include "ps_application.h"

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

static bool __psVulkanRendererCreateCommandBuffer(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkan->graphicsCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = vulkan->renderer.numInFlightFrames;

    static VkCommandBuffer commandBuffers[16] = {0};
    VkResult result = vkAllocateCommandBuffers(vulkan->device, &allocInfo, commandBuffers);
    PS_CHECK_VK_RESULT(result, "Failed to allocate command buffer\n");

    for (uint32_t i = 0; i < vulkan->renderer.numInFlightFrames; ++i)
        vulkan->renderer.resources[i].commandBuffer = commandBuffers[i];

    return true;
}

static bool __psVulkanRendererCreateSynchronizationObjects(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    // create the semaphores and fence for each in-flight frame
    for (uint32_t i = 0; i < vulkan->renderer.numInFlightFrames; ++i)
    {
        PS_CHECK(__psVulkanCreateSemaphore(vulkan->device, &vulkan->renderer.resources[i].imageAvailableSemaphore));
        PS_CHECK(__psVulkanCreateSemaphore(vulkan->device, &vulkan->renderer.resources[i].renderFinishedSemaphore));
        PS_CHECK(__psVulkanCreateFence(vulkan->device, &vulkan->renderer.resources[i].renderFence));
    }
    return true;
}

static void __psVulkanRendererDestroySynchronizationObjects(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    for (uint32_t i = 0; i < vulkan->renderer.numInFlightFrames; ++i)
    {
        vkDestroySemaphore(vulkan->device, vulkan->renderer.resources[i].imageAvailableSemaphore, NULL);
        vkDestroySemaphore(vulkan->device, vulkan->renderer.resources[i].renderFinishedSemaphore, NULL);
        vkDestroyFence(vulkan->device, vulkan->renderer.resources[i].renderFence, NULL);
    }
}

static bool __psVulkanRendererCreateRenderResources(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    vulkan->renderer.numInFlightFrames = PS_MIN(vulkan->swapchain.imageCount, 2);
    vulkan->renderer.currentFrameIndex = 0;

    // create the synchronization objects
    PS_CHECK(__psVulkanRendererCreateSynchronizationObjects(vulkan));
    PS_CHECK(__psVulkanRendererCreateCommandBuffer(vulkan));

    return true;
}

static void __psVulkanRendererDestroyRenderResources(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    __psVulkanRendererDestroySynchronizationObjects(vulkan);

    for (uint32_t i = 0; i < vulkan->renderer.numInFlightFrames; ++i)
        vkFreeCommandBuffers(vulkan->device, vulkan->graphicsCommandPool, 1, &vulkan->renderer.resources[i].commandBuffer);
}

static void __psVulkanRendererNextInflightFrame(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);
    vulkan->renderer.currentFrameIndex = (vulkan->renderer.currentFrameIndex + 1) % vulkan->renderer.numInFlightFrames;
}

static bool __psVulkanRendererHandleSwapchainResult(PS_Vulkan *vulkan, VkResult result) {
    PS_ASSERT(vulkan != NULL);

    switch (result)
    {
    case VK_SUCCESS:
        return true;
    case VK_SUBOPTIMAL_KHR:
        PS_LOG("Swapchain is suboptimal\n");
        vulkan->swapchain.swapchainRecreateRequired = true;
        __psVulkanRendererNextInflightFrame(vulkan);
        return false;
    case VK_ERROR_OUT_OF_DATE_KHR:
        PS_LOG("Swapchain is out of date\n");
        vulkan->swapchain.swapchainRecreateRequired = true;
        __psVulkanRendererNextInflightFrame(vulkan);
        return false;
    default:
        PS_LOG("Failed to acquire next image from swapchain: %d\n", result);
        exit(1);
    }

    return true;
}

bool psVulkanRendererCreate(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    // // create the command buffer and synchronization objects
    PS_CHECK(__psVulkanRendererCreateRenderResources(vulkan));
    PS_CHECK(psVulkanFramebufferCreate(vulkan, &vulkan->renderer.sceneFramebuffer, GAME_WIDTH, GAME_HEIGHT, true, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D32_SFLOAT));
    return true;
}

void psVulkanRendererDestroy(PS_Vulkan *vulkan)
{
    PS_ASSERT(vulkan != NULL);

    psVulkanFramebufferDestroy(vulkan, &vulkan->renderer.sceneFramebuffer);
    __psVulkanRendererDestroyRenderResources(vulkan);
}

void psVulkanRendererRender(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);

    PS_Vulkan *vulkan = &gameState->vulkan;
    PS_Window *window = &gameState->window;


    if (vulkan->swapchain.swapchainRecreateRequired)
    {
        PS_LOG("Swapchain recreate required\n");
        psVulkanSwapchainRecreate(vulkan, window);

        __psVulkanRendererDestroyRenderResources(vulkan);
        if (!__psVulkanRendererCreateRenderResources(vulkan))
        {
            PS_LOG("Failed to create render resources\n");
            exit(1);
        }

        return; // do not render this frame
    }

    uint32_t currentFrameIndex = vulkan->renderer.currentFrameIndex;
    vkWaitForFences(vulkan->device, 1, &vulkan->renderer.resources[currentFrameIndex].renderFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vulkan->device, 1, &vulkan->renderer.resources[currentFrameIndex].renderFence);

    uint32_t imageIndex = 0;
    VkResult result = psVulkanSwapchainAcquireNextImage(vulkan, &imageIndex, vulkan->renderer.resources[currentFrameIndex].imageAvailableSemaphore);
    if (!__psVulkanRendererHandleSwapchainResult(vulkan, result))
    {
        PS_LOG("Failed to acquire next image from swapchain\n");
        return;
    }

    VkCommandBuffer commandBuffer = vulkan->renderer.resources[currentFrameIndex].commandBuffer;
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = NULL;
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to begin command buffer: %d\n", result);
        vkResetFences(vulkan->device, 1, &vulkan->renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(vulkan);
        return; // do not render this frame
    }

    if(!psScenesRender(gameState)) {
        PS_LOG("Failed to render scenes pass\n");
        vkResetFences(vulkan->device, 1, &vulkan->renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(vulkan);
        return; // do not render this frame
    }

    if(!psVulkanPresentationRender(&gameState->vulkan, imageIndex)) {
        PS_LOG("Failed to render presentation pass\n");
        vkResetFences(vulkan->device, 1, &vulkan->renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(vulkan);
        return; // do not render this frame
    }

    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to end command buffer: %d\n", result);
        vkResetFences(vulkan->device, 1, &vulkan->renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(vulkan);
        return; // do not render this frame
    }


    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vulkan->renderer.resources[currentFrameIndex].imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vulkan->renderer.resources[currentFrameIndex].renderFinishedSemaphore;

    result = vkQueueSubmit(vulkan->graphicsQueue, 1, &submitInfo, vulkan->renderer.resources[currentFrameIndex].renderFence);
    if (result != VK_SUCCESS)
    {
        PS_LOG("Failed to submit command buffer: %d\n", result);
        vkResetFences(vulkan->device, 1, &vulkan->renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(vulkan);
        return; // do not render this frame
    }

    result = psVulkanSwapchainPresent(vulkan, imageIndex, vulkan->renderer.resources[currentFrameIndex].renderFinishedSemaphore);
    if (!__psVulkanRendererHandleSwapchainResult(vulkan, result))
    {
        PS_LOG("Failed to present swapchain image\n");
        return; // do not render this frame
    }

    __psVulkanRendererNextInflightFrame(vulkan);
}
