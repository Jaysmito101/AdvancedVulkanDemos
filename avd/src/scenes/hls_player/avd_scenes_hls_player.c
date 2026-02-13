#include "scenes/hls_player/avd_scenes_hls_player.h"

#include "GLFW/glfw3.h"
#include "audio/avd_audio.h"
#include "audio/avd_audio_core.h"
#include "avd_application.h"
#include "core/avd_base.h"
#include "core/avd_types.h"
#include "core/avd_utils.h"
#include "math/avd_vector_non_simd.h"
#include "pico/picoStream.h"
#include "scenes/avd_scenes.h"
#include "scenes/hls_player/avd_scene_hls_player_context.h"
#include "scenes/hls_player/avd_scene_hls_player_segment_store.h"
#include "scenes/hls_player/avd_scene_hls_stream.h"
#include "vulkan/avd_vulkan_base.h"
#include "vulkan/avd_vulkan_image.h"
#include "vulkan/avd_vulkan_video.h"
#include "vulkan/video/avd_vulkan_video_decoder.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <winnls.h>

typedef struct {
    uint32_t activeSources;
    int32_t indexCount;
    int32_t textureIndices;
    int32_t pad1;
    AVD_Vector4 cameraPosition;
    AVD_Vector4 cameraDirection;
} AVD_HLSPlayerPushConstants;

// TODO: a better way would be to acutally feed these to the shader via
// push constants
static AVD_Vector3 __avdHLSSceneSourcePositions[] = {
    {-40.0, 12.0, 15.0},
    {-20.0, 12.0, -8.0},
    {20.0, 12.0, -5.0},
    {40.0, 12.0, 18.0}};

static AVD_SceneHLSPlayer *__avdSceneGetTypePtr(union AVD_Scene *scene)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(scene->type == AVD_SCENE_TYPE_HLS_PLAYER);
    return &scene->hlsPlayer;
}

static bool __avdSceneHLSPlayerFreeSources(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    (void)appState;
    for (AVD_Size i = 0; i < scene->sourceCount; i++) {
        avdSceneHLSPlayerContextDestroy(&appState->vulkan, &appState->audio, &scene->sources[i].player);
    }
    memset(scene->sources, 0, sizeof(scene->sources));
    scene->sourceCount = 0;
    return true;
}

static bool __avdSceneHLSPlayerLoadSourcesFromPath(AVD_AppState *appState, AVD_SceneHLSPlayer *scene, const char *path)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(path != NULL);

    __avdSceneHLSPlayerFreeSources(appState, scene);

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
            scene->sources[sourceIndex].refreshIntervalMs = 10000.0f;
            scene->sources[sourceIndex].lastRefreshed     = -10000.0f;

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

static bool __avdSceneHLSPlayerRequestSourceUpdate(AVD_AppState *appState, AVD_SceneHLSPlayer *scene, AVD_UInt32 source)
{
    AVD_ASSERT(scene != NULL);
    AVD_ASSERT(source < scene->sourceCount);
    AVD_CHECK_MSG(
        avdHLSWorkerPoolSendSourceTask(
            &scene->workerPool,
            source,
            scene->sourcesHash,
            scene->sources[source].url),
        "Failed to send source download task");
    scene->sources[source].lastRefreshed = (AVD_Float)appState->framerate.currentTime;
    return true;
}

#ifdef AVD_SCENE_HLS_PLAYER_SAVE_SEGMENTS_TO_DISK
static bool __avdSceneHLSPlayerSaveSegmentToDisk(AVD_HLSSegmentAVData *avData)
{
    AVD_ASSERT(avData != NULL);

    static char buffer[64] = {0};
    snprintf(buffer, sizeof(buffer), "hls_segments/source_%zu/aac_adts", avData->source);
    AVD_CHECK(avdCreateDirectoryIfNotExists(buffer));
    snprintf(buffer, sizeof(buffer), "hls_segments/source_%zu/h264", avData->source);
    AVD_CHECK(avdCreateDirectoryIfNotExists(buffer));

    snprintf(buffer, sizeof(buffer), "hls_segments/source_%zu/h264/%zu.h264", avData->source, avData->segmentId);
    if (!avdWriteBinaryFile(buffer, avData->h264Buffer, avData->h264Size)) {
        AVD_LOG_WARN("Failed to write HLS segment to disk: %s", buffer);
    }

    snprintf(buffer, sizeof(buffer), "hls_segments/source_%zu/aac_adts/%zu.aac", avData->source, avData->segmentId);
    if (!avdWriteBinaryFile(buffer, avData->aacBuffer, avData->aacSize)) {
        AVD_LOG_WARN("Failed to write HLS segment to disk: %s", buffer);
    }

    return true;
}
#endif // AVD_SCENE_HLS_PLAYER_SAVE_SEGMENTS_TO_DISK

static bool __avdSceneHLSPlayerUpdateSources(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    AVD_Float time = (AVD_Float)appState->framerate.currentTime;

    for (AVD_Size i = 0; i < scene->sourceCount; i++) {
        AVD_SceneHLSPlayerSource *source = &scene->sources[i];
        if (!source->active) {
            continue;
        }

        if (source->lastRefreshed + source->refreshIntervalMs < time) {
            AVD_CHECK(__avdSceneHLSPlayerRequestSourceUpdate(appState, scene, (AVD_UInt32)i));
            continue;
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
            avdHLSSegmentAVDataFree(&payload.avData);
            continue;
        }

        AVD_SceneHLSPlayerSource *source = &scene->sources[payload.avData.source];

        source->refreshIntervalMs = AVD_MIN(source->refreshIntervalMs, payload.avData.duration);

#ifdef AVD_SCENE_HLS_PLAYER_SAVE_SEGMENTS_TO_DISK
        __avdSceneHLSPlayerSaveSegmentToDisk(&payload.avData);
#endif

        AVD_LOG_INFO("received ready segment %u uration: %.3f at %.3f", payload.avData.segmentId, payload.avData.duration, (AVD_Float)appState->framerate.currentTime);
        AVD_CHECK(
            avdSceneHLSPlayerContextAddSegment(
                &appState->vulkan,
                &appState->audio,
                &source->player,
                payload.avData));
    }

    return true;
}

static bool __avdSceneHLSPlayerUpdateContexts(AVD_AppState *appState, AVD_SceneHLSPlayer *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_Float time = (AVD_Float)appState->framerate.currentTime;

    for (AVD_Size i = 0; i < scene->sourceCount; i++) {
        AVD_SceneHLSPlayerSource *source = &scene->sources[i];

        if (source->currentlyPlayingSegmentId != 0 && !avdSceneHLSPlayerContextIsFed(&source->player) && time - source->lastRefreshed >= 1.0f) {
            AVD_LOG_WARN("HLS Player context ran out of data to decode/play!");
            __avdSceneHLSPlayerRequestSourceUpdate(appState, scene, (AVD_UInt32)i);
        }

        if (!source->player.initialized) {
            continue;
        }

        AVD_CHECK(avdSceneHLSPlayerContextUpdate(&appState->vulkan, &appState->audio, &source->player, time));

        source->currentlyPlayingSegmentId = avdSceneHLSPlayerContextGetCurrentlyPlayingSegmentId(&source->player);

        AVD_VulkanVideoDecodedFrame *frame = NULL;
        if (avdSceneHLSPlayerContextTryAcquireFrame(&source->player, &frame)) {
            source->firstBound = true;
            AVD_LOG_WARN_DEBOUNCED(1000, "decoded frame %zu at time (time: %.3f/%zu, curren time: %.3f) %.3f seconds", frame->index, frame->timestampSeconds, frame->absoluteDisplayOrder, avdSceneHLSPlayerContextGetTime(&source->player), time);

            VkWriteDescriptorSet descriptorWrite[2] = {
                {
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet          = appState->vulkan.bindlessDescriptorSet,
                    .dstBinding      = (AVD_UInt32)AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .dstArrayElement = (AVD_UInt32)i * 2 + 0,
                    .descriptorCount = 1,
                    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo      = &frame->ycbcrSubresource.raw.luma.descriptorImageInfo,
                },
                {
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet          = appState->vulkan.bindlessDescriptorSet,
                    .dstBinding      = (AVD_UInt32)AVD_VULKAN_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .dstArrayElement = (AVD_UInt32)i * 2 + 1,
                    .descriptorCount = 1,
                    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo      = &frame->ycbcrSubresource.raw.chroma.descriptorImageInfo,
                },
            };
            vkUpdateDescriptorSets(appState->vulkan.device, 2, descriptorWrite, 0, NULL);
        }

        AVD_Float distanceFromCamera      = avdVec3Length(avdVec3Subtract(__avdHLSSceneSourcePositions[i], scene->cameraPosition));
        source->player.audioPlayer.volume = 10.0f / (distanceFromCamera * distanceFromCamera);
    }

    return true;
}

static bool avdSceneHLSPlayerCheckIntegrity(struct AVD_AppState *appState, const char **statusMessage)
{
    AVD_ASSERT(statusMessage != NULL);
    *statusMessage = NULL;

    if (!appState->vulkan.supportedFeatures.videoDecode) {
        *statusMessage = "HLS Player scene is not supported on this GPU (video decode not supported). Or its disabled via CMake options.";
        return false;
    }

    // we currently not using the conversion variant of the samplers
    // if (!appState->vulkan.supportedFeatures.ycbcrConversion) {
    //     *statusMessage = "HLS Player scene is not supported on this GPU (YCbCr conversion not supported).";
    //     return false;
    // }

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

    api->displayName = "Live TV/HLS Player";
    api->id          = "HLSPlayer";

    return true;
}

bool avdSceneHLSPlayerInit(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    memset(hlsPlayer->sources, 0, sizeof(hlsPlayer->sources));

    hlsPlayer->isSupported = appState->vulkan.supportedFeatures.videoDecode; // && appState->vulkan.supportedFeatures.ycbcrConversion;

    AVD_CHECK(avdRenderableTextCreate(
        &hlsPlayer->title,
        &appState->fontRenderer,
        &appState->vulkan,
        "ShantellSansBold",
        hlsPlayer->isSupported ? "Live TV/Livestreams Player" : "Live TV/Livestreams Player (Not Supported on this GPU)",
        64.0f));
    AVD_CHECK(avdRenderableTextCreate(
        &hlsPlayer->info,
        &appState->fontRenderer,
        &appState->vulkan,
        "RobotoCondensedRegular",
        "Move around enjoy the shows!",
        24.0f));

    hlsPlayer->sceneWidth      = (float)appState->renderer.sceneFramebuffer.width;
    hlsPlayer->sceneHeight     = (float)appState->renderer.sceneFramebuffer.height;
    hlsPlayer->cameraPosition  = avdVec3(0.0f, 15.0f, 60.0f);
    hlsPlayer->cameraYaw       = AVD_PI; // looking toward -Z
    hlsPlayer->cameraPitch     = 0.0f;
    hlsPlayer->cameraDirection = avdVec3(
        cosf(hlsPlayer->cameraPitch) * sinf(hlsPlayer->cameraYaw),
        sinf(hlsPlayer->cameraPitch),
        cosf(hlsPlayer->cameraPitch) * cosf(hlsPlayer->cameraYaw));

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
        AVD_CHECK(__avdSceneHLSPlayerLoadSourcesFromPath(appState, hlsPlayer, "assets/scene_hls_player/sources.txt"));
        AVD_LOG_INFO("Loaded HLS sources from sources.txt");
    }

    hlsPlayer->vulkan = &appState->vulkan;

    AVD_CHECK(avdHLSURLPoolInit(&hlsPlayer->urlPool));
    AVD_CHECK(avdHLSMediaCacheInit(&hlsPlayer->mediaCache));
    AVD_CHECK(avdHLSWorkerPoolInit(&hlsPlayer->workerPool, &hlsPlayer->urlPool, &hlsPlayer->mediaCache, hlsPlayer));

    return true;
}

void avdSceneHLSPlayerDestroy(struct AVD_AppState *appState, union AVD_Scene *scene)
{
    AVD_ASSERT(appState != NULL);
    AVD_ASSERT(scene != NULL);

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    avdHLSWorkerPoolDestroy(&hlsPlayer->workerPool);
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

    AVD_SceneHLSPlayer *hlsPlayer = __avdSceneGetTypePtr(scene);

    if (event->type == AVD_INPUT_EVENT_KEY) {
        if (event->key.key == GLFW_KEY_ESCAPE && event->key.action == GLFW_PRESS) {
            avdSceneManagerSwitchToScene(
                &appState->sceneManager,
                AVD_SCENE_TYPE_MAIN_MENU,
                appState);
        }
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_MOVE && appState->input.mouseButtonState[GLFW_MOUSE_BUTTON_LEFT]) {
        float deltaX = appState->input.mouseDeltaX;
        float deltaY = appState->input.mouseDeltaY;

        const float sensitivity = 0.6f;
        hlsPlayer->cameraYaw += deltaX * sensitivity;
        hlsPlayer->cameraPitch -= deltaY * sensitivity;

        hlsPlayer->cameraPitch = AVD_CLAMP(hlsPlayer->cameraPitch, -AVD_PI * 0.45f, AVD_PI * 0.45f);

        hlsPlayer->cameraDirection.x = cosf(hlsPlayer->cameraPitch) * sinf(hlsPlayer->cameraYaw);
        hlsPlayer->cameraDirection.y = sinf(hlsPlayer->cameraPitch);
        hlsPlayer->cameraDirection.z = cosf(hlsPlayer->cameraPitch) * cosf(hlsPlayer->cameraYaw);

        hlsPlayer->cameraDirection = avdVec3Normalize(hlsPlayer->cameraDirection);
    } else if (event->type == AVD_INPUT_EVENT_MOUSE_SCROLL) {
        hlsPlayer->cameraPosition = avdVec3Add(hlsPlayer->cameraPosition, avdVec3Scale(hlsPlayer->cameraDirection, 0.5f * event->mouseScroll.y));
    } else if (event->type == AVD_INPUT_EVENT_DRAG_N_DROP) {
        if (event->dragNDrop.count > 0) {
            AVD_LOG_INFO("Trying to load sources from: %s", event->dragNDrop.paths[0]);

            avdHLSWorkerPoolFlush(&hlsPlayer->workerPool);
            avdHLSURLPoolClear(&hlsPlayer->urlPool);
            __avdSceneHLSPlayerLoadSourcesFromPath(appState, hlsPlayer, event->dragNDrop.paths[0]);
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
    AVD_CHECK(__avdSceneHLSPlayerUpdateContexts(appState, hlsPlayer));

    {
        const float moveSpeed = 40.0f * (AVD_Float)appState->framerate.deltaTime;
        AVD_Vector3 forward   = hlsPlayer->cameraDirection;
        AVD_Vector3 up        = avdVec3(0.0f, 1.0f, 0.0f);
        AVD_Vector3 right     = avdVec3Normalize(avdVec3Cross(forward, up));

        if (appState->input.keyState[GLFW_KEY_W]) {
            forward.y                 = 0.0f;
            hlsPlayer->cameraPosition = avdVec3Add(hlsPlayer->cameraPosition, avdVec3Scale(forward, moveSpeed));
        }
        if (appState->input.keyState[GLFW_KEY_S]) {
            forward.y                 = 0.0f;
            hlsPlayer->cameraPosition = avdVec3Add(hlsPlayer->cameraPosition, avdVec3Scale(forward, -moveSpeed));
        }
        if (appState->input.keyState[GLFW_KEY_A]) {
            forward.y                 = 0.0f;
            hlsPlayer->cameraPosition = avdVec3Add(hlsPlayer->cameraPosition, avdVec3Scale(right, -moveSpeed));
        }
        if (appState->input.keyState[GLFW_KEY_D]) {
            forward.y                 = 0.0f;
            hlsPlayer->cameraPosition = avdVec3Add(hlsPlayer->cameraPosition, avdVec3Scale(right, moveSpeed));
        }
        if (appState->input.keyState[GLFW_KEY_Q]) {
            hlsPlayer->cameraPosition = avdVec3Add(hlsPlayer->cameraPosition, avdVec3Scale(up, moveSpeed));
        }
        if (appState->input.keyState[GLFW_KEY_E]) {
            hlsPlayer->cameraPosition = avdVec3Add(hlsPlayer->cameraPosition, avdVec3Scale(up, -moveSpeed));
        }
    }

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
        int32_t textureIndices = 0;
        for (AVD_Size i = 0; i < 4; i++) {
            int32_t texIdx = 5;
            if (i < hlsPlayer->sourceCount && hlsPlayer->sources[i].firstBound) {
                texIdx = (int32_t)i;
            }
            textureIndices |= (texIdx & 0xFF) << (i * 8);
        }

        AVD_HLSPlayerPushConstants pushConstants = {
            .activeSources   = 0,
            .textureIndices  = textureIndices,
            .cameraPosition  = avdVec4FromVec3(hlsPlayer->cameraPosition, 1.0f),
            .cameraDirection = avdVec4FromVec3(hlsPlayer->cameraDirection, 0.0f),
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
