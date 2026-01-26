#include "scenes/hls_player/avd_scenes_hls_player.h"

#include "avd_application.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "font/avd_font.h"
#include "math/avd_math_base.h"
#include "pico/picoLog.h"
#include "scenes/avd_scenes.h"
#include "vulkan/avd_vulkan.h"
#include "vulkan/avd_vulkan_video.h"

#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t activeSources;
    int32_t indexCount;
    int32_t pad0;
    int32_t pad1;
} AVD_HLSPlayerPushConstants;

static AVD_SceneHLSPlayer *__avdSceneGetTypePtr(union AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_HLS_PLAYER);
    return &scene->hlsPlayer;
}

static bool __avdSceneHLSPlayerLoadSourcesFromPath(AVD_SceneHLSPlayer *scene, const char *path)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(path != NULL);

    char *fileData  = NULL;
    size_t fileSize = 0;
    if (!avdReadBinaryFile(path, (void **)&fileData, &fileSize)) {
        AVD_LOG_ERROR("Failed to read HLS sources file: %s", path);
        return false;
    }

    const char *lineStart = fileData;

    AVD_Size sourceIndex = 0;
    while (*lineStart && sourceIndex < AVD_SCENE_HLS_PLAYER_MAX_SOURCES) {
        const char *lineEnd = strchr(lineStart, '\n');
        if (!lineEnd)
            lineEnd = strchr(lineStart, '\0');

        size_t lineLength = lineEnd - lineStart;
        if (lineLength > 0) {
            if (lineLength >= sizeof(scene->sources[sourceIndex].url)) {
                AVD_LOG_WARN("HLS source URL too long, truncating: %.*s", (int)lineLength, lineStart);
                lineLength = sizeof(scene->sources[sourceIndex].url) - 1;
            }
            if (!avdIsStringAURL(lineStart)) {
                AVD_LOG_WARN("Invalid URL format for HLS source, skipping: %.*s", (int)lineLength, lineStart);
                lineStart = (*lineEnd == '\0') ? lineEnd : lineEnd + 1;
                continue;
            }
            AVD_LOG_INFO("Loaded HLS source: %.*s", (int)lineLength, lineStart);
            strncpy(scene->sources[sourceIndex].url, lineStart, lineLength);
            scene->sources[sourceIndex].url[lineLength]         = '\0';
            scene->sources[sourceIndex].active                  = true;
            scene->sources[sourceIndex].refreshIntervalMs       = 10000.0f;
            scene->sources[sourceIndex].lastRefreshed           = -10000.0f;
            scene->sources[sourceIndex].currentSegmentIndex     = 0;
            scene->sources[sourceIndex].currentSegmentStartTime = 0.0f;
            scene->sources[sourceIndex].currentsegmentDuration  = 0.0f;

            sourceIndex++;
        }

        lineStart = (*lineEnd == '\0') ? lineEnd : lineEnd + 1;
    }
    scene->sourceCount = (AVD_UInt32)sourceIndex;
    scene->sourcesHash = avdHashBuffer(scene->sources, sizeof(AVD_SceneHLSPlayerSource) * scene->sourceCount);

    AVD_LOG_INFO("Total HLS sources loaded: %d [hash: 0x%08X]", scene->sourceCount, scene->sourcesHash);

    AVD_FREE(fileData);
    return true;
}

static bool __avdSceneHLSPlayerFreeSources(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    (void)appState;
    for (AVD_Size i = 0; i < scene->sourceCount; i++) {
        if (scene->sources[i].active) {
            scene->sources[i].active = false;
        }
    }
    scene->sourceCount = 0;
    return true;
}

static bool __avdSceneHLSPlayerUpdateSources(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    AVD_Float time = (AVD_Float)appState->framerate.currentTime;

    for (AVD_Size i = 0; i < scene->sourceCount; i++) {
        AVD_SceneHLSPlayerSource *source = &scene->sources[i];
        if (!source->active) {
            continue;
        }

        if (source->lastRefreshed + source->refreshIntervalMs < time) {
            AVD_CHECK_MSG(
                avdHLSWorkerPoolSendSourceTask(
                    &scene->workerPool,
                    (AVD_UInt32)i,
                    scene->sourcesHash,
                    source->url),
                "Failed to send source download task");
            source->lastRefreshed = time;
            continue;
        }

        if (source->currentSegmentStartTime + source->currentsegmentDuration < time) {
            AVD_UInt32 currentSegmentId = source->currentSegmentIndex;
            if (currentSegmentId == 0 || avdHLSSegmentStoreHasSegment(&scene->segmentStore, (AVD_UInt32)i, currentSegmentId)) {
                AVD_UInt32 nextSegmentId = 0;
                if (avdHLSSegmentStoreFindNextSegment(&scene->segmentStore, (AVD_UInt32)i, currentSegmentId, &nextSegmentId)) {
                    if (currentSegmentId != 0) {
                        avdHLSSegmentStoreRelease(&scene->segmentStore, (AVD_UInt32)i, currentSegmentId);
                    }

                    AVD_H264Video *video = avdHLSSegmentStoreAcquire(&scene->segmentStore, (AVD_UInt32)i, nextSegmentId);
                    AVD_CHECK_MSG(video != NULL, "Failed to acquire video for segment %u of source index %u", nextSegmentId, (AVD_UInt32)i);

                    AVD_LOG_INFO("HLS Source %u playing segment %u [prev segment: %u] %f", (AVD_UInt32)i, nextSegmentId, currentSegmentId, time);
                    // avdH264VideoDebugPrint(video);
                    avdH264VideoDestroy(video);

                    source->currentSegmentIndex     = nextSegmentId;
                    source->currentSegmentStartTime = time;

                    AVD_HLSSegmentSlot *slot = avdHLSSegmentStoreGetSlot(&scene->segmentStore, (AVD_UInt32)i, nextSegmentId);
                    AVD_CHECK_MSG(slot != NULL, "Failed to get slot for segment %u of source index %u", nextSegmentId, (AVD_UInt32)i);
                    source->currentsegmentDuration = slot->duration;
                }
            }
        }
    }

    return true;
}

static bool __avdSceneHLSPlayerReceiveReadySegments(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    (void)appState;

    AVD_HLSReadyPayload payload = {0};
    while (avdHLSWorkerPoolReceiveReadySegment(&scene->workerPool, &payload)) {
        if (payload.sourcesHash != scene->sourcesHash) {
            AVD_LOG_WARN("HLS Main Thread received segment for outdated sources hash 0x%08X (current: 0x%08X), discarding", payload.sourcesHash, scene->sourcesHash);
            if (payload.video) {
                avdH264VideoDestroy(payload.video);
            }
            continue;
        }

        scene->sources[payload.sourceIndex].refreshIntervalMs = AVD_MIN(
            scene->sources[payload.sourceIndex].refreshIntervalMs,
            payload.duration);

        bool isSegmentOld         = scene->sources[payload.sourceIndex].currentSegmentIndex > payload.segmentId;
        bool segmentAlreadyLoaded = avdHLSSegmentStoreHasSegment(&scene->segmentStore, payload.sourceIndex, payload.segmentId);

        if (isSegmentOld || segmentAlreadyLoaded) {
            if (payload.video) {
                avdH264VideoDestroy(payload.video);
            }
            continue;
        }

        // AVD_LOG_VERBOSE("Received ready segment %u for source index %u (duration: %f s)", payload.segmentId, payload.sourceIndex, payload.duration);
        if (!avdHLSSegmentStoreCommit(&scene->segmentStore, payload.sourceIndex, payload.segmentId, payload.video, payload.duration)) {
            AVD_LOG_WARN("HLS Main Thread could not commit segment %u for source index %u", payload.segmentId, payload.sourceIndex);
        }
    }

    return true;
}

static void printBinary(AVD_UInt32 n)
{
    for (int i = sizeof(n) * 8 - 1; i >= 0; i--) {
        putchar((n & (1 << i)) ? '1' : '0');
    }
    putchar('\n');
}

static AVD_UInt32 __avdSceneHLSPlayerUpdateActiveSources(AVD_SceneHLSPlayer *scene)
{
    AVD_UInt32 currentSegmentIds[AVD_SCENE_HLS_PLAYER_MAX_SOURCES];
    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_MAX_SOURCES; i++) {
        currentSegmentIds[i] = scene->sources[i].active ? scene->sources[i].currentSegmentIndex : 0;
    }
    return avdHLSSegmentStoreGetActiveSourcesBitfield(&scene->segmentStore, currentSegmentIds);
}

static bool avdSceneHLSPlayerCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    if (!appState->vulkan.supportedFeatures.videoDecode) {
        *statusMessage = "HLS Player scene is not supported on this GPU (video decode not supported).";
        return false;
    }

    if (!avdCurlIsSupported()) {
        *statusMessage = "HLS Player scene requires curl cli to be installed and available in PATH.";
        return false;
    }

    if (!avdPathExists("assets/scene_hls_player/sources.txt")) {
        AVD_LOG_WARN("HLS Player default sources file not found: assets/scene_hls_player/sources.txt");
    }

    return true;
}

bool avdSceneHLSPlayerRegisterApi(AVD_SceneAPI *api)
{
    AVD_ASSERT(api != NULL);

    api->checkIntegrity = avdSceneHLSPlayerCheckIntegrity;
    api->init           = avdSceneHLSPlayerInit;
    api->render         = avdSceneHLSPlayerRender;
    api->update         = avdSceneHLSPlayerUpdate;
    api->destroy        = avdSceneHLSPlayerDestroy;
    api->load           = avdSceneHLSPlayerLoad;
    api->inputEvent     = avdSceneHLSPlayerInputEvent;

    api->displayName = "HLS Player";
    api->id          = "Bloom";

    return true;
}

bool avdSceneHLSPlayerInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    memset(hlsPlayer->sources, 0, sizeof(hlsPlayer->sources));

    hlsPlayer->isSupported = appState->vulkan.supportedFeatures.videoDecode;

    AVD_CHECK(avdRenderableTextCreate(
        &hlsPlayer->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        hlsPlayer->isSupported ? "HLS Player" : "HLS Player (Not Supported on this GPU)",
        64.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &hlsPlayer->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Copy a url to a HLS stream and click the window and press P.",
        24.0f));

    hlsPlayer->sceneWidth  = (float)appState->renderer.sceneFramebuffer.width;
    hlsPlayer->sceneHeight = (float)appState->renderer.sceneFramebuffer.height;

    if (!hlsPlayer->isSupported) {
        return true;
    }

    AVD_VulkanPipelineCreationInfo pipelineCreationInfo = {0};
    avdPipelineUtilsPipelineCreationInfoInit(&pipelineCreationInfo);
    pipelineCreationInfo.enableDepthTest = true;
    AVD_CHECK(avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
        &hlsPlayer->pipelineLayout,
        &hlsPlayer->pipeline,
        appState->vulkan.device,
        &appState->vulkan.bindlessDescriptorSetLayout,
        1,
        sizeof(AVD_HLSPlayerPushConstants),
        appState->renderer.sceneFramebuffer.renderPass,
        (AVD_UInt32)appState->renderer.sceneFramebuffer.colorAttachments.count,
        "HLSPlayerVert",
        "HLSPlayerFrag",
        NULL,
        &pipelineCreationInfo));

    if (avdPathExists("assets/scene_hls_player/sources.txt")) {
        AVD_CHECK(__avdSceneHLSPlayerLoadSourcesFromPath(hlsPlayer, "assets/scene_hls_player/sources.txt"));
        AVD_LOG_INFO("Loaded HLS sources from sources.txt");
    }

    hlsPlayer->vulkan = &appState->vulkan;

    AVD_CHECK(avdHLSURLPoolInit(&hlsPlayer->urlPool));
    AVD_CHECK(avdHLSMediaCacheInit(&hlsPlayer->mediaCache));
    AVD_CHECK(avdHLSSegmentStoreInit(&hlsPlayer->segmentStore, hlsPlayer->sourceCount > 0 ? hlsPlayer->sourceCount : AVD_SCENE_HLS_PLAYER_MAX_SOURCES));
    AVD_CHECK(avdHLSWorkerPoolInit(&hlsPlayer->workerPool, &hlsPlayer->urlPool, &hlsPlayer->mediaCache, &hlsPlayer->segmentStore, hlsPlayer->vulkan, hlsPlayer));

    return true;
}

void avdSceneHLSPlayerDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    //    avdVulkanVideoDecoderDestroy(&appState->vulkan, &hlsPlayer->vulkanVideo);

    avdHLSWorkerPoolDestroy(&hlsPlayer->workerPool);
    avdHLSSegmentStoreDestroy(&hlsPlayer->segmentStore);
    avdHLSMediaCacheDestroy(&hlsPlayer->mediaCache);
    avdHLSURLPoolDestroy(&hlsPlayer->urlPool);

    __avdSceneHLSPlayerFreeSources(appState, hlsPlayer);

    avdRenderableTextDestroy(&hlsPlayer->title, &appState->vulkan);
    avdRenderableTextDestroy(&hlsPlayer->info, &appState->vulkan);

    if (!hlsPlayer->isSupported) {
        return;
    }

    vkDestroyPipeline(appState->vulkan.device, hlsPlayer->pipeline, NULL);
    vkDestroyPipelineLayout(appState->vulkan.device, hlsPlayer->pipelineLayout, NULL);
}

bool avdSceneHLSPlayerLoad(struct AVD_AppState *appState, union AVD_Scene *scene, const char **statusMessage, float *progress)
{
    (void)appState;
    (void)scene;
    AVD_ASSERT(statusMessage != NULL);
    AVD_ASSERT(progress != NULL);

    *progress      = 1.0f;
    *statusMessage = "HLS Player scene loaded successfully.";
    return true;
}

void avdSceneHLSPlayerInputEvent(struct AVD_AppState *appState, union AVD_Scene *scene, AVD_InputEvent *event)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(event != NULL);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        } else if (event->key.key == GLFW_KEY_P && event->key.action == GLFW_PRESS) {
            AVD_LOG_INFO("P pressed - would start playing HLS stream if implemented %s.", glfwGetClipboardString(appState->window.window));
        }
    } else if (event->type == AVD_INPUT_EVENT_DRAG_N_DROP) {
        if (event->dragNDrop.count > 0) {
            AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);
            AVD_LOG_INFO("Trying to load sources from: %s", event->dragNDrop.paths[0]);

            avdHLSWorkerPoolFlush(&hlsPlayer->workerPool);
            avdHLSSegmentStoreClear(&hlsPlayer->segmentStore);
            avdHLSURLPoolClear(&hlsPlayer->urlPool);
            __avdSceneHLSPlayerFreeSources(appState, hlsPlayer);
            __avdSceneHLSPlayerLoadSourcesFromPath(hlsPlayer, event->dragNDrop.paths[0]);
        }
    }
}

bool avdSceneHLSPlayerUpdate(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    if (!hlsPlayer->isSupported) {
        return true;
    }

    AVD_CHECK(__avdSceneHLSPlayerUpdateSources(appState, hlsPlayer));
    AVD_CHECK(__avdSceneHLSPlayerReceiveReadySegments(appState, hlsPlayer));

    return true;
}

bool avdSceneHLSPlayerRender(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    VkCommandBuffer commandBuffer = avdVulkanRendererGetCurrentCmdBuffer(&appState->renderer);
    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    AVD_CHECK(avdBeginSceneRenderPass(commandBuffer, &appState->renderer));

    float titleWidth, titleHeight;
    float infoWidth, infoHeight;
    avdRenderableTextGetSize(&hlsPlayer->title, &titleWidth, &titleHeight);
    avdRenderableTextGetSize(&hlsPlayer->info, &infoWidth, &infoHeight);

    if (hlsPlayer->isSupported) {
        AVD_HLSPlayerPushConstants pushConstants = {
            .activeSources = __avdSceneHLSPlayerUpdateActiveSources(hlsPlayer),
        };
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hlsPlayer->pipeline);
        vkCmdPushConstants(commandBuffer, hlsPlayer->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hlsPlayer->pipelineLayout, 0, 1, &appState->vulkan.bindlessDescriptorSet, 0, NULL);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    }

    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &hlsPlayer->title,
        commandBuffer,
        ((float)appState->renderer.sceneFramebuffer.width - titleWidth) / 2.0f,
        titleHeight + 10.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);
    avdRenderText(
        &appState->vulkan,
        &appState->fontRenderer,
        &hlsPlayer->info,
        commandBuffer,
        10.0f, 10.0f + infoHeight,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        appState->renderer.sceneFramebuffer.width,
        appState->renderer.sceneFramebuffer.height);

    AVD_CHECK(avdEndSceneRenderPass(commandBuffer));

    return true;
}
