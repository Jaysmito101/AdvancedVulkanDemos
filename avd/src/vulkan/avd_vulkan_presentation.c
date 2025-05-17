#include "vulkan/avd_vulkan.h"
#include "shader/avd_shader.h"
#include "scenes/avd_scenes.h"

// Define the push constant struct matching the shader layout
typedef struct AVD_VulkanPresentationPushConstants
{
    float windowWidth;
    float windowHeight;
    float framebufferWidth;
    float framebufferHeight;
    
    float sceneLoadingProgress;
    float time;
    float pad0;
    float pad1;
} AVD_VulkanPresentationPushConstants;

bool avdVulkanPresentationInit(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain, AVD_FontRenderer *fontRenderer)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(vulkan != NULL);
    
    AVD_CHECK(avdCreateDescriptorSetLayout(
        &presentation->descriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));
    AVD_CHECK(avdPipelineUtilsCreateGraphicsPipelineLayout(
        &presentation->pipelineLayout,
        vulkan->device,
        &presentation->descriptorSetLayout, 1,
        sizeof(AVD_VulkanPresentationPushConstants)));
    AVD_CHECK(avdPipelineUtilsCreateGenericGraphicsPipeline(
        &presentation->pipeline,
        presentation->pipelineLayout,
        vulkan->device,
        swapchain->renderPass,
        "PresentationVert",
        "PresentationFrag"));

    AVD_CHECK(avdRenderableTextCreate(
        &presentation->loadingText,
        fontRenderer,
        vulkan,
        "OpenSansRegular",
        "Loading...",
        24.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &presentation->loadingStatusText,
        fontRenderer,
        vulkan,
        "OpenSansRegular",
        "No status",
        16.0f));
    return true;
}

void avdVulkanPresentationDestroy(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(vulkan != NULL);

    avdRenderableTextDestroy(&presentation->loadingStatusText, vulkan);
    avdRenderableTextDestroy(&presentation->loadingText, vulkan);
    vkDestroyPipeline(vulkan->device, presentation->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, presentation->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, presentation->descriptorSetLayout, NULL);
}

bool avdVulkanPresentationRender(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanRenderer *renderer, AVD_VulkanSwapchain *swapchain, AVD_SceneManager *sceneManager, AVD_FontRenderer *fontRenderer, uint32_t imageIndex)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(swapchain != NULL);

    uint32_t currentFrameIndex = renderer->currentFrameIndex;
    VkCommandBuffer commandBuffer = renderer->resources[currentFrameIndex].commandBuffer;

    static VkClearValue defaultClearValues[2] = {
        {.color = {.float32 = {0.0f, 0.0f, 0.0f}}},
        {.depthStencil = {.depth = 1.0f, .stencil = 0}}};

    AVD_CHECK(avdBeginRenderPass(
        commandBuffer,
        swapchain->renderPass,
        swapchain->framebuffers[imageIndex],
        swapchain->extent.width,
        swapchain->extent.height,
        defaultClearValues,
        AVD_ARRAY_COUNT(defaultClearValues)));

    // Populate the push constant struct
    AVD_VulkanPresentationPushConstants pushConstants = {
        .windowWidth = (float)swapchain->extent.width,
        .windowHeight = (float)swapchain->extent.height,
        .framebufferWidth = (float)renderer->sceneFramebuffer.width,
        .framebufferHeight = (float)renderer->sceneFramebuffer.height,
        .sceneLoadingProgress = sceneManager->sceneLoadingProgress,
        .time = (float)glfwGetTime(),
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentation->pipeline);
    vkCmdPushConstants(commandBuffer, presentation->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AVD_VulkanPresentationPushConstants), &pushConstants);
    VkDescriptorSet descriptorSetsToBind[] = {
        renderer->sceneFramebuffer.colorAttachment.descriptorSet, // Set 0
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentation->pipelineLayout, 0, AVD_ARRAY_COUNT(descriptorSetsToBind), descriptorSetsToBind, 0, NULL);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    // NOTE: This wont work for now, as the font renderer pipeline only works with
    //       the scene framebuffer and not the swapchain framebuffer, this will be fixed later?
#if 0
    if (!sceneManager->isSceneLoaded)
    {
        static char loadingText[256];
        snprintf(loadingText, sizeof(loadingText), "Loading... [%.2f%%]", sceneManager->sceneLoadingProgress * 100.0f);
        AVD_CHECK(avdRenderableTextUpdate(
            &presentation->loadingText,
            fontRenderer,
            vulkan,
            loadingText));

        avdRenderText(
            vulkan,
            fontRenderer,
            &presentation->loadingText,
            commandBuffer,
            100.0f, 100.0f,
            1.0f,
            1.0f, 1.0f, 0.0f, 1.0f,
            swapchain->extent.width, swapchain->extent.height);

        if (sceneManager->sceneLoadingStatusMessage != NULL)
        {
            AVD_CHECK(avdRenderableTextUpdate(
                &presentation->loadingStatusText,
                fontRenderer,
                vulkan,
                sceneManager->sceneLoadingStatusMessage));

            avdRenderText(
                vulkan,
                fontRenderer,
                &presentation->loadingStatusText,
                commandBuffer,
                100.0f, 500.0f,
                1.0f,
                1.0f, 1.0f, 0.0f, 1.0f,
                swapchain->extent.width, swapchain->extent.height);
        }
    }
#endif

    AVD_CHECK(avdEndRenderPass(commandBuffer));
    return true;
}