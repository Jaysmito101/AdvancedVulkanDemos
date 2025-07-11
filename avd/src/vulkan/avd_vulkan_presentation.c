#include "scenes/avd_scenes.h"
#include "shader/avd_shader.h"
#include "vulkan/avd_vulkan.h"

// Define the push constant struct matching the shader layout
typedef struct AVD_VulkanPresentationPushConstants {
    float windowWidth;
    float windowHeight;
    float framebufferWidth;
    float framebufferHeight;

    float sceneLoadingProgress;
    float time;
    float pad0;
    float pad1;
} AVD_VulkanPresentationPushConstants;

bool avdVulkanPresentationInit(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanSwapchain *swapchain, AVD_FontManager *fontManager)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(vulkan != NULL);

    AVD_CHECK(avdCreateDescriptorSetLayout(
        &presentation->descriptorSetLayout,
        vulkan->device,
        (VkDescriptorType[]){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT));
    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &presentation->pipelineLayout,
        &presentation->pipeline,
        vulkan->device,
        (VkDescriptorSetLayout[]){presentation->descriptorSetLayout},
        1,
        sizeof(AVD_VulkanPresentationPushConstants),
        swapchain->renderPass,
        1,
        "PresentationVert",
        "PresentationFrag",
        NULL,
        NULL));
    AVD_CHECK(avdFontRendererCreate(
        &presentation->presentationFontRenderer,
        vulkan,
        fontManager,
        swapchain->renderPass));
    AVD_CHECK(avdRenderableTextCreate(
        &presentation->loadingText,
        &presentation->presentationFontRenderer,
        vulkan,
        "OpenSansRegular",
        "Loading...",
        24.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &presentation->loadingStatusText,
        &presentation->presentationFontRenderer,
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
    avdFontRendererDestroy(&presentation->presentationFontRenderer, vulkan);
    vkDestroyPipeline(vulkan->device, presentation->pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, presentation->pipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, presentation->descriptorSetLayout, NULL);
}

bool avdVulkanPresentationRender(AVD_VulkanPresentation *presentation, AVD_Vulkan *vulkan, AVD_VulkanRenderer *renderer, AVD_VulkanSwapchain *swapchain, AVD_SceneManager *sceneManager, uint32_t imageIndex)
{
    AVD_ASSERT(presentation != NULL);
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(renderer != NULL);
    AVD_ASSERT(swapchain != NULL);

    uint32_t currentFrameIndex    = renderer->currentFrameIndex;
    VkCommandBuffer commandBuffer = renderer->resources[currentFrameIndex].commandBuffer;

    static VkClearValue defaultClearValues[2] = {
        {.color = {.float32 = {0.0f, 0.0f, 0.0f}}},
        {.depthStencil = {.depth = 1.0f, .stencil = 0}}};

    AVD_CHECK(avdBeginRenderPass(
        commandBuffer,
        swapchain->renderPass,
        swapchain->framebuffer,
        (const VkImageView[]){swapchain->imageViews[imageIndex]},
        1,
        swapchain->extent.width,
        swapchain->extent.height,
        defaultClearValues,
        AVD_ARRAY_COUNT(defaultClearValues)));

    // Populate the push constant struct
    AVD_VulkanPresentationPushConstants pushConstants = {
        .windowWidth          = (float)swapchain->extent.width,
        .windowHeight         = (float)swapchain->extent.height,
        .framebufferWidth     = (float)renderer->sceneFramebuffer.width,
        .framebufferHeight    = (float)renderer->sceneFramebuffer.height,
        .sceneLoadingProgress = sceneManager->sceneLoadingProgress,
        .time                 = (float)glfwGetTime(),
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentation->pipeline);
    vkCmdPushConstants(commandBuffer, presentation->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AVD_VulkanPresentationPushConstants), &pushConstants);
    VkDescriptorSet descriptorSetsToBind[] = {
        avdVulkanFramebufferGetColorAttachment(&renderer->sceneFramebuffer, 0)->descriptorSet, // Set 0
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentation->pipelineLayout, 0, AVD_ARRAY_COUNT(descriptorSetsToBind), descriptorSetsToBind, 0, NULL);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    if (!sceneManager->isSceneLoaded)
    {
        AVD_FontRenderer *fontRenderer = &presentation->presentationFontRenderer;
        static char loadingText[256];
        snprintf(loadingText, sizeof(loadingText), "Loading... [%.2f%%]", sceneManager->sceneLoadingProgress * 100.0f);
        AVD_CHECK(avdRenderableTextUpdate(
            &presentation->loadingText,
            fontRenderer,
            vulkan,
            loadingText));

        float textWidth, textHeight;
        avdRenderableTextGetSize(&presentation->loadingText, &textWidth, &textHeight);

        avdRenderText(
            vulkan,
            fontRenderer,
            &presentation->loadingText,
            commandBuffer,
            (swapchain->extent.width - textWidth) / 2.0f,
            (swapchain->extent.height / 2.0f - textHeight) / 2.0f,
            1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            swapchain->extent.width, swapchain->extent.height);

        if (sceneManager->sceneLoadingStatusMessage != NULL)
        {
            AVD_CHECK(avdRenderableTextUpdate(
                &presentation->loadingStatusText,
                fontRenderer,
                vulkan,
                sceneManager->sceneLoadingStatusMessage));
            avdRenderableTextGetSize(&presentation->loadingStatusText, &textWidth, &textHeight);

            avdRenderText(
                vulkan,
                fontRenderer,
                &presentation->loadingStatusText,
                commandBuffer,
                (swapchain->extent.width - textWidth) / 2.0f,
                (3.0f * swapchain->extent.height / 2.0f - textHeight) / 2.0f,
                1.0f,
                1.0f, 1.0f, 1.0f, 1.0f,
                swapchain->extent.width, swapchain->extent.height);
        }
    }

    AVD_CHECK(avdEndRenderPass(commandBuffer));
    return true;
}