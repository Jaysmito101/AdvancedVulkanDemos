#include "avd_application.h"
#include "scenes/avd_scenes.h"
#include "vulkan/avd_vulkan_renderer.h"

static bool __avdSetupDescriptors(VkDescriptorSetLayout *layout, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(vulkan != NULL);
    AVD_ASSERT(layout != NULL);

    VkDescriptorSetLayoutBinding sceneFramebufferBinding = {0};
    sceneFramebufferBinding.binding                      = 0;
    sceneFramebufferBinding.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sceneFramebufferBinding.descriptorCount              = 1;
    sceneFramebufferBinding.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo sceneFramebufferLayoutInfo = {0};
    sceneFramebufferLayoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sceneFramebufferLayoutInfo.bindingCount                    = 1;
    sceneFramebufferLayoutInfo.pBindings                       = &sceneFramebufferBinding;

    VkResult sceneLayoutResult = vkCreateDescriptorSetLayout(vulkan->device, &sceneFramebufferLayoutInfo, NULL, layout);
    AVD_CHECK_VK_RESULT(sceneLayoutResult, "Failed to create scene framebuffer descriptor set layout");
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, *layout, "[DescriptorSetLayout][Scene]:MainMenu/DescriptorSetLayout");
    return true;
}

static bool __avdSetupMainMenuCard(const char *imageAsset, const char *title, AVD_SceneType targetSceneType, AVD_SceneMainMenuCard *card, AVD_Vulkan *vulkan, AVD_FontRenderer *fontRenderer, VkDescriptorSetLayout layout)
{
    AVD_ASSERT(card != NULL);
    AVD_ASSERT(imageAsset != NULL);
    AVD_ASSERT(title != NULL);

    card->targetSceneType = targetSceneType;
    AVD_CHECK(avdVulkanImageLoadFromAsset(vulkan, imageAsset, &card->thumbnailImage, NULL));
    AVD_CHECK(avdRenderableTextCreate(
        &card->title,
        fontRenderer,
        vulkan,
        "RobotoCondensedRegular",
        title,
        28.0f));

    VkDescriptorSetAllocateInfo allocateInfo = {0};
    allocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool              = vulkan->descriptorPool;
    allocateInfo.descriptorSetCount          = 1;
    allocateInfo.pSetLayouts                 = &layout;
    AVD_CHECK_VK_RESULT(vkAllocateDescriptorSets(vulkan->device, &allocateInfo, &card->descriptorSet), "Failed to allocate descriptor set");

    char debugName[128];
    snprintf(debugName, sizeof(debugName), "[DescriptorSet][Scene]:MainMenu/Card/%s", title);
    AVD_DEBUG_VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET, card->descriptorSet, debugName);

    VkWriteDescriptorSet writeDescriptorSet = {0};
    AVD_CHECK(avdWriteImageDescriptorSet(&writeDescriptorSet, card->descriptorSet, 0, &card->thumbnailImage.defaultSubresource.descriptorImageInfo));
    vkUpdateDescriptorSets(vulkan->device, 1, &writeDescriptorSet, 0, NULL);

    return true;
}

static void __avdDestroyMainMenuCard(AVD_SceneMainMenuCard *card, AVD_Vulkan *vulkan)
{
    AVD_ASSERT(card != NULL);
    AVD_ASSERT(vulkan != NULL);

    avdVulkanImageDestroy(vulkan, &card->thumbnailImage);
    avdRenderableTextDestroy(&card->title, vulkan);
}

static bool __avdSetupMainMenuCards(AVD_SceneMainMenu *mainMenu, AVD_AppState *appState)
{
    AVD_ASSERT(mainMenu != NULL);
    AVD_ASSERT(appState != NULL);

    AVD_SceneMainMenuCard *card = NULL;
    static char title[64];
    mainMenu->cardCount = 0;

    AVD_SceneAPI *api = &appState->sceneManager.api[0];

    for (int i = 0; i < AVD_SCENE_TYPE_COUNT; i++) {
        if (i == AVD_SCENE_TYPE_MAIN_MENU) {
            continue; // Skip the main menu itself
        }

        card = &mainMenu->cards[mainMenu->cardCount++];
        AVD_CHECK(
            __avdSetupMainMenuCard(
                api[i].id,
                api[i].displayName,
                (AVD_SceneType)i,
                card,
                &appState->vulkan,
                &appState->fontRenderer,
                mainMenu->descriptorSetLayout));
    }

    return true;
}

static AVD_SceneMainMenu *__avdSceneGetTypePtr(AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_MAIN_MENU);
    return &scene->mainMenu;
}

bool avdSceneMainMenuCheckIntegrity(AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    // Main menu always passes integrity check
    // as the main menu assets are directly embedded
    // in the application binary.
    return true;
}

bool avdSceneMainMenuRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneMainMenuCheckIntegrity;
    api->init           = avdSceneMainMenuInit;
    api->render         = avdSceneMainMenuRender;
    api->update         = avdSceneMainMenuUpdate;
    api->destroy        = avdSceneMainMenuDestroy;
    api->load           = avdSceneMainMenuLoad;
    api->inputEvent     = avdSceneMainMenuInputEvent;

    // These wont be use for the main menu scene, but they are required for the scene API to be valid
    api->displayName = "Main Menu";
    api->id          = "DDGIPlaceholder";

    return true;
}

bool avdSceneMainMenuInit(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);
    AVD_LOG_INFO("Initializing main menu scene");
    mainMenu->loadingCount = 0;
    mainMenu->currentPage  = 0;
    mainMenu->hoveredCard  = -1;

    AVD_CHECK(__avdSetupDescriptors(&mainMenu->descriptorSetLayout, &appState->vulkan));
    AVD_CHECK(__avdSetupMainMenuCards(mainMenu, appState));

    AVD_CHECK(avdRenderableTextCreate(
        &mainMenu->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Advanced Vulkan Demos",
        72.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &mainMenu->creditsText,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Made with Love by Jaysmito Mukherjee",
        36.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &mainMenu->githubLinkText,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "(https://github.com/Jaysmito101/AdvancedVulkanDemos)",
        16.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &mainMenu->pageNumberText,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Page 1/1",
        16.0f));

    return true;
}

void avdSceneMainMenuDestroy(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);

    AVD_LOG_INFO("Destroying main menu scene");
    avdRenderableTextDestroy(&mainMenu->title, &appState->vulkan);
    avdRenderableTextDestroy(&mainMenu->creditsText, &appState->vulkan);
    avdRenderableTextDestroy(&mainMenu->githubLinkText, &appState->vulkan);
    avdRenderableTextDestroy(&mainMenu->pageNumberText, &appState->vulkan);
    vkDestroyDescriptorSetLayout(appState->vulkan.device, mainMenu->descriptorSetLayout, NULL);
    for (uint32_t i = 0; i < mainMenu->cardCount; i++)
        __avdDestroyMainMenuCard(&mainMenu->cards[i], &appState->vulkan);
}

bool avdSceneMainMenuLoad(AVD_AppState *appState, AVD_Scene *scene, const char **statusMessage, float *progress)
{
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);
    // nothing to load really here but some busy waiting
    // just as an example of how to use the loading progress and status message
    if (mainMenu->loadingCount < 4) {
        mainMenu->loadingCount++;
        static char buffer[256];
        snprintf(buffer, sizeof(buffer), "Loading main menu scene: %d%%", mainMenu->loadingCount * 100 / 4);
        *statusMessage = buffer;
        *progress      = (float)mainMenu->loadingCount / 4.0f;
        avdSleep(100);
        return false;
    }

    *statusMessage = NULL;
    *progress      = 1.0f;

    return true;
}

void avdSceneMainMenuInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            AVD_LOG_INFO("Exiting main menu scene");
            appState->running = false;
        } else if (event->key.key == GLFW_KEY_RIGHT && event->key.action == GLFW_PRESS) {
            mainMenu->currentPage++;
            if (mainMenu->currentPage >= (mainMenu->cardCount + 5) / 6)
                mainMenu->currentPage = 0;
        } else if (event->key.key == GLFW_KEY_LEFT && event->key.action == GLFW_PRESS) {
            if (mainMenu->currentPage > 0)
                mainMenu->currentPage--;
            else
                mainMenu->currentPage = (mainMenu->cardCount + 5) / 6 - 1;
        }
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_BUTTON) {
        if (event->mouseButton.button == GLFW_MOUSE_BUTTON_LEFT && event->mouseButton.action == GLFW_PRESS) {
            if (mainMenu->hoveredCard != -1) {
                AVD_SceneMainMenuCard *card = &mainMenu->cards[mainMenu->hoveredCard];
                avdSceneManagerSwitchToScene(
                    &appState->sceneManager,
                    card->targetSceneType,
                    appState);
            }
        }
    }
}

bool avdSceneMainMenuUpdate(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);
    return true;
}

bool avdSceneMainMenuRender(AVD_AppState *appState, AVD_Scene *scene)
{
    AVD_SceneMainMenu *mainMenu = __avdSceneGetTypePtr(scene);

    AVD_Vulkan *vulkan           = &appState->vulkan;
    AVD_VulkanRenderer *renderer = &appState->renderer;

    VkCommandBuffer commandBuffer = avdVulkanRendererGetCurrentCmdBuffer(&appState->renderer);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));
    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][Scene]:MainMenu/Render");

    // AVD_LOG_VERBOSE("Rendering main menu scene");

    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][Scene]:MainMenu/RenderTitle");
    float titleWidth, titleHeight;
    float creditsWidth, creditsHeight;
    float githubLinkWidth, githubLinkHeight;
    {
        // render the title
        avdRenderableTextGetSize(&mainMenu->title, &titleWidth, &titleHeight);
        avdRenderableTextGetSize(&mainMenu->creditsText, &creditsWidth, &creditsHeight);
        avdRenderableTextGetSize(&mainMenu->githubLinkText, &githubLinkWidth, &githubLinkHeight);

        avdRenderText(
            &appState->vulkan,
            &appState->fontRenderer,
            &mainMenu->title,
            commandBuffer,
            ((float)renderer->sceneFramebuffer.width - titleWidth) / 2.0f, titleHeight,
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            renderer->sceneFramebuffer.width,
            renderer->sceneFramebuffer.height);

        avdRenderText(
            &appState->vulkan,
            &appState->fontRenderer,
            &mainMenu->creditsText,
            commandBuffer,
            ((float)renderer->sceneFramebuffer.width - creditsWidth) / 2.0f, titleHeight + creditsHeight + 10.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            renderer->sceneFramebuffer.width,
            renderer->sceneFramebuffer.height);

        avdRenderText(
            &appState->vulkan,
            &appState->fontRenderer,
            &mainMenu->githubLinkText,
            commandBuffer,
            ((float)renderer->sceneFramebuffer.width - githubLinkWidth) / 2.0f, titleHeight + creditsHeight + 10.0f + githubLinkHeight + 10.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            renderer->sceneFramebuffer.width,
            renderer->sceneFramebuffer.height);
    }
    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer); // Scene/MainMenu/RenderTitle

    float minX        = 0.0f;
    float minY        = (titleHeight + creditsHeight + githubLinkHeight + 40.0f);
    float frameWidth  = (float)renderer->sceneFramebuffer.width;
    float frameHeight = (float)renderer->sceneFramebuffer.height - minY;

    float cardWidth      = (frameWidth * 0.8f) / 3.0f - 20.0f;
    float cardHeight     = (frameHeight * 0.8f) / 2.0f - 80.0f;
    float allCardsWidth  = cardWidth * 3.0f + 20.0f * 2.0f;
    float allCardsHeight = cardHeight * 2.0f + 80.0f * 2.0f;
    float offsetX        = (frameWidth - allCardsWidth) / 2.0f;
    float offsetY        = (frameHeight - allCardsHeight) / 2.0f;

    AVD_DEBUG_VK_CMD_BEGIN_LABEL(commandBuffer, NULL, "[Cmd][Scene]:MainMenu/RenderUi");

    avdUiBegin(
        commandBuffer,
        &appState->ui,
        appState,
        (float)renderer->sceneFramebuffer.width,
        (float)renderer->sceneFramebuffer.height - minY,
        0.0f, minY,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    const uint32_t numCardsPerPage = 6;
    uint32_t currentOffset         = mainMenu->currentPage * numCardsPerPage;
    uint32_t totalPages            = (mainMenu->cardCount + numCardsPerPage - 1) / numCardsPerPage;

    float mouseX = (appState->input.mouseX * 0.5f + 0.5f) * frameWidth;
    float mouseY = frameHeight - minY / 2.0f - (appState->input.mouseY * 0.5f + 0.5f) * frameHeight;

    mainMenu->hoveredCard = -1;
    for (uint32_t i = 0; i < numCardsPerPage; i++) {
        if (currentOffset + i >= mainMenu->cardCount)
            break;

        AVD_SceneMainMenuCard *card = &mainMenu->cards[currentOffset + i];
        float x                     = (float)(i % 3) * (cardWidth + 20.0f) + offsetX;
        float y                     = (float)(i / 3) * (cardHeight + 80.0f) + offsetY;

        AVD_VulkanImage *image = &card->thumbnailImage;

        if (mouseX >= x && mouseX <= x + cardWidth && mouseY >= y && mouseY <= y + cardHeight) {
            mainMenu->hoveredCard = currentOffset + i;
            avdUiDrawRect(
                commandBuffer,
                &appState->ui,
                appState,
                x - 15.0f, y - 15.0f,
                cardWidth + 30.0f, cardHeight + 30.0f,
                1.0f, 1.0f, 1.0f, 1.0f,
                card->descriptorSet, image->info.width, image->info.height);
        } else {
            avdUiDrawRect(
                commandBuffer,
                &appState->ui,
                appState,
                x, y,
                cardWidth, cardHeight,
                0.3f, 0.3f, 0.3f, 1.0f,
                card->descriptorSet, image->info.width, image->info.height);
        }

        float cardTitleWidth, cardTitleHeight;
        avdRenderableTextGetSize(&card->title, &cardTitleWidth, &cardTitleHeight);
        avdRenderText(
            &appState->vulkan,
            &appState->fontRenderer,
            &card->title,
            commandBuffer,
            x + (cardWidth - cardTitleWidth) / 2.0f, y + cardHeight + cardTitleHeight + minY + 10.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            renderer->sceneFramebuffer.width,
            renderer->sceneFramebuffer.height);
    }

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer); // Scene/MainMenu/RenderUi

    // Draw the page indicator
    static char pageIndicator[32];
    snprintf(pageIndicator, sizeof(pageIndicator), "Page %d/%d", mainMenu->currentPage + 1, totalPages);
    AVD_CHECK(avdRenderableTextUpdate(
        &mainMenu->pageNumberText,
        &appState->fontRenderer,
        &appState->vulkan,
        pageIndicator));

    float pageIndicatorWidth, pageIndicatorHeight;
    avdRenderableTextGetSize(&mainMenu->pageNumberText, &pageIndicatorWidth, &pageIndicatorHeight);

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &mainMenu->pageNumberText,
        commandBuffer,
        (frameWidth - pageIndicatorWidth) * 0.5f, frameHeight - pageIndicatorHeight - 10.0f + minY,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        renderer->sceneFramebuffer.width,
        renderer->sceneFramebuffer.height);

    avdUiEnd(
        commandBuffer,
        &appState->ui,
        appState);

    AVD_DEBUG_VK_CMD_END_LABEL(commandBuffer);
    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}
