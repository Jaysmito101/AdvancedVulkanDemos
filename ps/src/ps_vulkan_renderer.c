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

static bool __psVulkanSceneCreateDescriptors(PS_GameState *gameState) {
    PS_ASSERT(gameState != NULL);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[1] = {0};
    descriptorSetLayoutBindings[0].binding = 0;
    descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBindings[0].descriptorCount = 1;
    descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {0};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = PS_ARRAY_COUNT(descriptorSetLayoutBindings);
    descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings;

    VkResult result = vkCreateDescriptorSetLayout(gameState->vulkan.device, &descriptorSetLayoutInfo, NULL, &gameState->vulkan.renderer.sceneFramebufferColorDescriptorSetLayout);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to create descriptor set layout\n");
        return false;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {0};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = gameState->vulkan.descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &gameState->vulkan.renderer.sceneFramebufferColorDescriptorSetLayout;

    result = vkAllocateDescriptorSets(gameState->vulkan.device, &descriptorSetAllocateInfo, &gameState->vulkan.renderer.sceneFramebufferColorDescriptorSet);
    if (result != VK_SUCCESS) {
        PS_LOG("Failed to allocate descriptor set\n");
        return false;
    }

    VkWriteDescriptorSet writeDescriptorSet = {0};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = gameState->vulkan.renderer.sceneFramebufferColorDescriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet.pImageInfo = &gameState->vulkan.renderer.sceneFramebuffer.colorAttachment.image.descriptorImageInfo;
    writeDescriptorSet.pBufferInfo = NULL;
    writeDescriptorSet.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(gameState->vulkan.device, 1, &writeDescriptorSet, 0, NULL);

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

    if (!psVulkanFramebufferCreate(gameState, &gameState->vulkan.renderer.sceneFramebuffer, GAME_WIDTH, GAME_HEIGHT, true, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D32_SFLOAT))
    {
        PS_LOG("Failed to create Vulkan framebuffer\n");
        return false;
    }

    if (!__psVulkanSceneCreateDescriptors(gameState)) {
        PS_LOG("Failed to create scene descriptors\n");
        return false;
    }

    return true;
}

void psVulkanRendererDestroy(PS_GameState *gameState)
{
    PS_ASSERT(gameState != NULL);
    
    vkDestroyDescriptorSetLayout(gameState->vulkan.device, gameState->vulkan.renderer.sceneFramebufferColorDescriptorSetLayout, NULL);
    gameState->vulkan.renderer.sceneFramebufferColorDescriptorSetLayout = VK_NULL_HANDLE;

    psVulkanFramebufferDestroy(gameState, &gameState->vulkan.renderer.sceneFramebuffer);
    
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

    if(!psScenesRender(gameState)) {
        PS_LOG("Failed to render scenes pass\n");
        vkResetFences(gameState->vulkan.device, 1, &gameState->vulkan.renderer.resources[currentFrameIndex].renderFence);
        __psVulkanRendererNextInflightFrame(gameState);
        return; // do not render this frame
    }

    if(!psVulkanPresentationRender(gameState, imageIndex)) {
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
