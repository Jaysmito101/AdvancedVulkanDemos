#include "scenes/hls_player/avd_scenes_hls_player.h"
#include "avd_application.h"
#include "core/avd_base.h"
#include "core/avd_utils.h"
#include "font/avd_font.h"
#include "math/avd_math_base.h"
#include "pico/picoM3U8.h"
#include "pico/picoThreads.h"
#include "scenes/avd_scenes.h"
#include <objidl.h>
#include <stdint.h>

typedef struct {
    uint32_t activeSources; // each source has got 4 bits (4 * 8 = 32)
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

static AVD_UInt32 __avdSceneHLSPlayerUpdateActiveSources(AVD_SceneHLSPlayer *scene)
{
    AVD_UInt32 result = 0;
    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_MAX_SOURCES; i++) {
        result |= (AVD_UInt32)(scene->sources[i].active ? 1 : 0) << i * 4;
    }
    return result;
}

static bool __avdSceneHLSPlayerLoadSourcesFromPath(AVD_SceneHLSPlayer *scene, const char *path)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(path != NULL);

    // path should have a text file with sources listed line by line
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
            scene->sources[sourceIndex].url[lineLength]   = '\0';
            scene->sources[sourceIndex].active            = true;
            scene->sources[sourceIndex].refreshIntervalMs = 10000000.0f; // by default have this very high
            scene->sources[sourceIndex].lastRefreshed     = -1.0 * scene->sources[sourceIndex].refreshIntervalMs;
            sourceIndex++;
        }

        lineStart = (*lineEnd == '\0') ? lineEnd : lineEnd + 1;
    }
    scene->sourceCount = (AVD_UInt32)sourceIndex;
    scene->sourcesHash = avdHashBuffer(scene->sources, sizeof(AVD_SceneHLSPlayerSource) * scene->sourceCount);

    AVD_LOG_INFO("Total HLS sources loaded: %d [hash: 0x%08X]", scene->sourceCount, scene->sourcesHash);

    free(fileData);
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

        if (source->lastRefreshed + source->refreshIntervalMs > time) {
            continue;
        }

        static AVD_SceneHLSPlayerSourceWorkerPayload payload = {0};
        memcpy(payload.url, source->url, sizeof(payload.url));
        payload.sourcesHash = scene->sourcesHash;
        payload.sourceIndex = (AVD_UInt32)i;
        AVD_CHECK_MSG(picoThreadChannelSend(scene->sourceDownloadChannel, &payload), "Failed to send source download payload to worker thread");

        source->lastRefreshed = time;
        return true;
    }

    return true;
}

static void __avdSceneHLSSourceDownloadWorker(void *arg)
{
    AVD_SceneHLSPlayer *scene          = (AVD_SceneHLSPlayer *)arg;
    scene->sourceDownloadWorkerRunning = true;
    AVD_LOG_INFO("HLS Data Worker thread started with thread ID: %llu.", picoThreadGetCurrentId());
    AVD_SceneHLSPlayerSourceWorkerPayload payload = {0};
    picoM3U8Playlist sourcePlaylist               = NULL;
    while (scene->sourceDownloadWorkerRunning) {

        if (picoThreadChannelReceive(scene->sourceDownloadChannel, &payload, 1000)) {
            AVD_LOG_VERBOSE("HLS Source Download Worker received payload for source index %u, url: %s", payload.sourceIndex, payload.url);

            char *data = NULL;
            if (!avdCurlFetchStringContent(payload.url, &data, NULL)) {
                AVD_LOG_ERROR("Failed to fetch HLS playlist for source: %s", payload.url);
                continue;
            }
            if (picoM3U8PlaylistParse(data, (uint32_t)strlen(data), &sourcePlaylist) != PICO_M3U8_RESULT_SUCCESS) {
                AVD_LOG_ERROR("Failed to parse HLS playlist for source: %s", payload.url);
                free(data);
                continue;
            }

            if (sourcePlaylist->type != PICO_M3U8_PLAYLIST_TYPE_MEDIA) {
                AVD_LOG_ERROR("Master playlists are not supported for yet!: %s", payload.url);
                picoM3U8PlaylistDestroy(sourcePlaylist);
                free(data);
            }
            AVD_LOG_VERBOSE("%s", data);
            picoM3U8PlaylistDebugPrint(sourcePlaylist);

            free(data);
        }
    }
    AVD_LOG_INFO("HLS Data Worker thread stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

static void __avdSceneHLSMediaDownloadWoker(void *args)
{
    AVD_SceneHLSPlayer *scene         = (AVD_SceneHLSPlayer *)args;
    scene->mediaDownloadWorkerRunning = true;
    AVD_LOG_INFO("HLS Media Download Worker thread started with thread ID: %llu.", picoThreadGetCurrentId());
    while (scene->mediaDownloadWorkerRunning) {
        picoThreadSleep(1000); // Sleep for 5 seconds before checking again
    }
    AVD_LOG_INFO("HLS Media Download Worker thread stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

static void __avdSceneHLSMediaDemuxWorker(void *args)
{
    AVD_SceneHLSPlayer *scene      = (AVD_SceneHLSPlayer *)args;
    scene->mediaDemuxWorkerRunning = true;
    AVD_LOG_INFO("HLS Media Demux Worker thread started with thread ID: %llu.", picoThreadGetCurrentId());
    while (scene->mediaDemuxWorkerRunning) {
        picoThreadSleep(1000); // Sleep for 5 seconds before checking again
    }
    AVD_LOG_INFO("HLS Media Demux Worker thread stopping with thread ID: %llu.", picoThreadGetCurrentId());
}

static bool __avdSceneHLSPlayerPrepWorkers(AVD_SceneHLSPlayer *scene)
{
    AVD_ASSERT(scene != NULL);

    // Create the thread channels to communicate between threads
    scene->sourceDownloadChannel = picoThreadChannelCreateUnbounded(128);
    AVD_CHECK_MSG(scene->sourceDownloadChannel != NULL, "Failed to create HLS source download channel");

    scene->mediaDownloadChannel = picoThreadChannelCreateUnbounded(128);
    AVD_CHECK_MSG(scene->mediaDownloadChannel != NULL, "Failed to create HLS media download channel");

    scene->mediaDemuxChannel = picoThreadChannelCreateUnbounded(128);
    AVD_CHECK_MSG(scene->mediaDemuxChannel != NULL, "Failed to create HLS media demux channel");

    scene->mediaReadyChannel = picoThreadChannelCreateUnbounded(128);
    AVD_CHECK_MSG(scene->mediaReadyChannel != NULL, "Failed to create HLS media ready channel");

    // Spin up the threads
    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_SOURCE_WORKERS; i++) {
        scene->sourceDownloadWorker[i] = picoThreadCreate(__avdSceneHLSSourceDownloadWorker, scene);
        AVD_CHECK_MSG(scene->sourceDownloadWorker[i] != NULL, "Failed to create HLS data worker thread");
    }

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DOWNLOAD_WORKERS; i++) {
        scene->mediaDownloadWorker[i] = picoThreadCreate(__avdSceneHLSMediaDownloadWoker, scene);
        AVD_CHECK_MSG(scene->mediaDownloadWorker[i] != NULL, "Failed to create HLS data worker thread");
    }

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DEMUX_WORKERS; i++) {
        scene->mediaDemuxWorker[i] = picoThreadCreate(__avdSceneHLSMediaDemuxWorker, scene);
        AVD_CHECK_MSG(scene->mediaDemuxWorker[i] != NULL, "Failed to create HLS data worker thread");
    }

    return true;
}

static void __avdSceneHLSPlayerWindDownWorkers(AVD_SceneHLSPlayer *scene)
{
    AVD_ASSERT(scene != NULL);

    scene->sourceDownloadWorkerRunning = false;
    scene->mediaDownloadWorkerRunning  = false;
    scene->mediaDemuxWorkerRunning     = false;

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_SOURCE_WORKERS; i++) {
        picoThreadDestroy(scene->sourceDownloadWorker[i]);
    }

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DOWNLOAD_WORKERS; i++) {
        picoThreadDestroy(scene->mediaDownloadWorker[i]);
    }

    for (AVD_Size i = 0; i < AVD_SCENE_HLS_PLAYER_NUM_MEDIA_DEMUX_WORKERS; i++) {
        picoThreadDestroy(scene->mediaDemuxWorker[i]);
    }

    picoThreadChannelDestroy(scene->sourceDownloadChannel);
    picoThreadChannelDestroy(scene->mediaDownloadChannel);
    picoThreadChannelDestroy(scene->mediaDemuxChannel);
    picoThreadChannelDestroy(scene->mediaReadyChannel);
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

    AVD_CHECK(__avdSceneHLSPlayerPrepWorkers(hlsPlayer));

    return true;
}

void avdSceneHLSPlayerDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    __avdSceneHLSPlayerWindDownWorkers(hlsPlayer);

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
            AVD_LOG_INFO("Trying to load sources from: %s", event->dragNDrop.paths[0]);
            __avdSceneHLSPlayerLoadSourcesFromPath(__avdSceneGetTypePtr(scene), event->dragNDrop.paths[0]);
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
