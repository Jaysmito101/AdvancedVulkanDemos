#include "vulkan/avd_vulkan_renderer.h"
#include "core/avd_base.h"

static bool __avdVulkanCreateSemaphore(VkDevice device, VkSemaphore *semaphore)
{
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result = vkCreateSemaphore(device, &semaphoreInfo, NULL, semaphore);
    if (result != VK_SUCCESS) {
        AVD_LOG_ERROR("Failed to create semaphore");
        return false;
    }

    return true;
}

static bool __avdVulkanCreateFence(VkDevice device, VkFence *fence)
{
    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result = vkCreateFence(device, &fenceInfo, NULL, fence);
    if (result != VK_SUCCESS) {
        AVD_LOG_ERROR("Failed to create fence");
        return false;
    }

    return true;
}

static bool __avdVulkanRendererCreateCommandBuffer(AVD_VulkanRendererResources *resources, AVD_Vulkan *vulkan, uint32_t numInFlightFrames)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(numInFlightFrames > 0);
    AVD_ASSERT(numInFlightFrames <= AVD_MAX_IN_FLIGHT_FRAMES);

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = vulkan->graphicsCommandPool;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount          = numInFlightFrames;

    static VkCommandBuffer commandBuffers[16] = {0};
    VkResult result                           = vkAllocateCommandBuffers(vulkan->device, &allocInfo, commandBuffers);
    AVD_CHECK_VK_RESULT(result, "Failed to allocate command buffer\n");

    for (uint32_t i = 0; i < numInFlightFrames; ++i)
        resources[i].commandBuffer = commandBuffers[i];

    return true;
}

static bool __avdVulkanRendererCreateSynchronizationObjects(AVD_VulkanRendererResources *resources, AVD_Vulkan *vulkan, uint32_t numInFlightFrames)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(numInFlightFrames > 0);
    AVD_ASSERT(numInFlightFrames <= AVD_MAX_IN_FLIGHT_FRAMES);

    memset(resources, 0, sizeof(AVD_VulkanRendererResources) * AVD_MAX_IN_FLIGHT_FRAMES);

    // create the semaphores and fence for each in-flight frame
    for (uint32_t i = 0; i < numInFlightFrames; ++i) {
        AVD_CHECK(__avdVulkanCreateSemaphore(vulkan->device, &resources[i].imageAvailableSemaphore));
        AVD_CHECK(__avdVulkanCreateSemaphore(vulkan->device, &resources[i].renderFinishedSemaphore));
        AVD_CHECK(__avdVulkanCreateFence(vulkan->device, &resources[i].renderFence));
    }
    return true;
}

static void __avdVulkanRendererDestroySynchronizationObjects(AVD_VulkanRendererResources *resources, AVD_Vulkan *vulkan, uint32_t numInFlightFrames)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(numInFlightFrames > 0);
    AVD_ASSERT(numInFlightFrames <= AVD_MAX_IN_FLIGHT_FRAMES);

    for (uint32_t i = 0; i < numInFlightFrames; ++i) {
        vkDestroySemaphore(vulkan->device, resources[i].imageAvailableSemaphore, NULL);
        vkDestroySemaphore(vulkan->device, resources[i].renderFinishedSemaphore, NULL);
        vkDestroyFence(vulkan->device, resources[i].renderFence, NULL);
    }

    memset(resources, 0, sizeof(AVD_VulkanRendererResources) * AVD_MAX_IN_FLIGHT_FRAMES);
}

static bool __avdVulkanRendererCreateRenderResources(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, uint32_t swapchainImageCount)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);

    renderer->numInFlightFrames = AVD_MIN(swapchainImageCount, 2);
    renderer->currentFrameIndex = 0;

    // create the synchronization objects
    AVD_CHECK(__avdVulkanRendererCreateSynchronizationObjects(renderer->resources, vulkan, renderer->numInFlightFrames));
    AVD_CHECK(__avdVulkanRendererCreateCommandBuffer(renderer->resources, vulkan, renderer->numInFlightFrames));

    return true;
}

static void __avdVulkanRendererDestroyRenderResources(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);

    __avdVulkanRendererDestroySynchronizationObjects(renderer->resources, vulkan, renderer->numInFlightFrames);

    for (uint32_t i = 0; i < renderer->numInFlightFrames; ++i)
        vkFreeCommandBuffers(vulkan->device, vulkan->graphicsCommandPool, 1, &renderer->resources[i].commandBuffer);
}

static void __avdVulkanRendererNextInflightFrame(AVD_VulkanRenderer *renderer)
{
    AVD_ASSERT(renderer != NULL);
    renderer->currentFrameIndex = (renderer->currentFrameIndex + 1) % renderer->numInFlightFrames;
}

static bool __avdVulkanRendererHandleSwapchainResult(AVD_VulkanRenderer *renderer, AVD_VulkanSwapchain *swapchain, VkResult result)
{
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(swapchain != NULL);

    switch (result) {
        case VK_SUCCESS:
            return true;
        case VK_SUBOPTIMAL_KHR:
            AVD_LOG_WARN("Swapchain is suboptimal");
            swapchain->swapchainRecreateRequired = true;
            __avdVulkanRendererNextInflightFrame(renderer);
            return false;
        case VK_ERROR_OUT_OF_DATE_KHR:
            AVD_LOG_WARN("Swapchain is out of date");
            swapchain->swapchainRecreateRequired = true;
            __avdVulkanRendererNextInflightFrame(renderer);
            return false;
        default:
            AVD_LOG_ERROR("Failed to acquire next image from swapchain: %d", result);
            exit(1);
    }

    return true;
}

bool avdVulkanRendererCreate(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain, uint32_t width, uint32_t height)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(swapchain != NULL);
    AVD_ASSERT(width > 0);
    AVD_ASSERT(height > 0);
    AVD_ASSERT(swapchain->swapchainReady);

    renderer->width  = width;
    renderer->height = height;

    // create the command buffer and synchronization objects
    AVD_CHECK(__avdVulkanRendererCreateRenderResources(renderer, vulkan, swapchain->imageCount));
    AVD_CHECK(avdVulkanFramebufferCreate(vulkan, &renderer->sceneFramebuffer, width, height, true, (VkFormat[]){VK_FORMAT_R32G32B32A32_SFLOAT}, 1, VK_FORMAT_D32_SFLOAT));
    return true;
}

void avdVulkanRendererDestroy(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);

    avdVulkanFramebufferDestroy(vulkan, &renderer->sceneFramebuffer);
    __avdVulkanRendererDestroyRenderResources(renderer, vulkan);
}

bool avdVulkanRendererRecreateResources(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(swapchain != NULL);

    __avdVulkanRendererDestroyRenderResources(renderer, vulkan);
    if (!__avdVulkanRendererCreateRenderResources(renderer, vulkan, swapchain->imageCount)) {
        AVD_LOG_ERROR("Failed to create render resources");
        return false;
    }

    return true;
}

bool avdVulkanRendererBegin(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(swapchain != NULL);

    uint32_t currentFrameIndex = renderer->currentFrameIndex;
    vkWaitForFences(vulkan->device, 1, &renderer->resources[currentFrameIndex].renderFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vulkan->device, 1, &renderer->resources[currentFrameIndex].renderFence);

    VkResult result = avdVulkanSwapchainAcquireNextImage(swapchain, vulkan, &renderer->currentImageIndex, renderer->resources[currentFrameIndex].imageAvailableSemaphore, VK_NULL_HANDLE);
    if (!__avdVulkanRendererHandleSwapchainResult(renderer, swapchain, result)) {
        AVD_LOG_ERROR("Failed to acquire next image from swapchain");
        return false;
    }

    VkCommandBuffer commandBuffer = renderer->resources[currentFrameIndex].commandBuffer;
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo         = NULL;
    result                             = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        AVD_LOG_ERROR("Failed to begin command buffer: %s", string_VkResult(result));
        vkResetFences(vulkan->device, 1, &renderer->resources[currentFrameIndex].renderFence);
        __avdVulkanRendererNextInflightFrame(renderer);
        return false; // do not render this frame
    }

    return true;
}

bool avdVulkanRendererEnd(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(swapchain != NULL);

    uint32_t currentFrameIndex    = renderer->currentFrameIndex;
    VkCommandBuffer commandBuffer = renderer->resources[currentFrameIndex].commandBuffer;

    VkResult result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        AVD_LOG_ERROR("Failed to end command buffer: %s", string_VkResult(result));
        vkResetFences(vulkan->device, 1, &renderer->resources[currentFrameIndex].renderFence);
        __avdVulkanRendererNextInflightFrame(renderer);
        return false; // do not render this frame
    }

    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo         = {0};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &renderer->resources[currentFrameIndex].imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask    = &waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &renderer->resources[currentFrameIndex].renderFinishedSemaphore;

    result = vkQueueSubmit(vulkan->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        AVD_LOG_ERROR("Failed to submit command buffer: %s", string_VkResult(result));
        vkResetFences(vulkan->device, 1, &renderer->resources[currentFrameIndex].renderFence);
        __avdVulkanRendererNextInflightFrame(renderer);
        return false; // do not render this frame
    }

    result = avdVulkanSwapchainPresent(swapchain, vulkan, renderer->currentImageIndex, renderer->resources[currentFrameIndex].renderFinishedSemaphore, renderer->resources[currentFrameIndex].renderFence);
    if (!__avdVulkanRendererHandleSwapchainResult(renderer, swapchain, result)) {
        AVD_LOG_ERROR("Failed to present swapchain image");
        return false; // do not render this frame
    }

    __avdVulkanRendererNextInflightFrame(renderer);

    return true;
}

bool avdVulkanRendererCancelFrame(AVD_VulkanRenderer *renderer, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);

    uint32_t currentFrameIndex = renderer->currentFrameIndex;
    vkResetFences(vulkan->device, 1, &renderer->resources[currentFrameIndex].renderFence);
    __avdVulkanRendererNextInflightFrame(renderer);

    return true;
}

VkCommandBuffer avdVulkanRendererGetCurrentCmdBuffer(AVD_VulkanRenderer* renderer)
{
    AVD_ASSERT(renderer != NULL);

    uint32_t currentFrameIndex = renderer->currentFrameIndex;
    return renderer->resources[currentFrameIndex].commandBuffer;
}
