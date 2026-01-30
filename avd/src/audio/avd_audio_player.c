#include "audio/avd_audio_player.h"
#include "audio/avd_audio_clip.h"
#include "audio/avd_audio_core.h"
#include "core/avd_utils.h"

static bool __avdAudioPlayerCallback(
    const void *inputBuffer,
    void *outputBuffer,
    AVD_Size framesPerBuffer,
    AVD_UInt32 statusFlags,
    void *userData)
{
    (void)inputBuffer;
    (void)statusFlags;

    AVD_AudiPlayerContext *ctx = (AVD_AudiPlayerContext *)userData;
    if (!ctx || !ctx->clip || !ctx->isPlaying) {
        AVD_Size size = framesPerBuffer * sizeof(float) * (ctx && ctx->clip ? ctx->clip->channels : 2);
        for (AVD_Size i = 0; i < size / sizeof(float); i++) {
            ((float *)outputBuffer)[i] = 0.0f;
        }
        return true;
    }

    AVD_AudioClip *clip           = ctx->clip;
    AVD_AudioPlayerConfig *config = &ctx->config;
    float *out                    = (float *)outputBuffer;

    AVD_Size channels     = clip->channels;
    AVD_Size totalSamples = clip->sampleCount;

    AVD_Float effectiveSpeed = config->speed * config->pitch;
    if (effectiveSpeed <= 0.0f)
        effectiveSpeed = 1.0f;

    for (AVD_Size frame = 0; frame < framesPerBuffer; frame++) {
        AVD_Float srcPosition = (AVD_Float)ctx->sampleOffset + (AVD_Float)frame * effectiveSpeed * (AVD_Float)channels;
        AVD_Size srcIndex     = (AVD_Size)srcPosition;

        if (srcIndex >= totalSamples) {
            if (config->loop) {
                ctx->sampleOffset = 0;
                srcIndex          = (frame * channels) % totalSamples;
            } else {
                for (AVD_Size remaining = frame; remaining < framesPerBuffer; remaining++) {
                    for (AVD_Size ch = 0; ch < channels; ch++) {
                        out[remaining * channels + ch] = 0.0f;
                    }
                }
                ctx->isPlaying = false;
                return true;
            }
        }

        for (AVD_Size ch = 0; ch < channels; ch++) {
            AVD_Float sample     = 0.0f;
            AVD_Size sampleIndex = srcIndex + ch;

            if (sampleIndex < totalSamples) {
                avdAudioClipSampleAtIndex(clip, sampleIndex, &sample);
            }

            sample *= config->volume;

            if (sample > 1.0f)
                sample = 1.0f;
            if (sample < -1.0f)
                sample = -1.0f;

            out[frame * channels + ch] = sample;
        }
    }

    ctx->sampleOffset += (AVD_Size)((AVD_Float)framesPerBuffer * effectiveSpeed * (AVD_Float)channels);

    return true;
}

bool avdAudioPlayerInit(AVD_Audio *audio, AVD_AudioPlayer *player)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);

    memset(player, 0, sizeof(AVD_AudioPlayer));
    player->audio        = audio;
    player->globalVolume = 1.0f;

    for (AVD_Size i = 0; i < AVD_AUDIO_PLAYER_MAX_CLIPS; i++) {
        player->clips[i].parent       = player;
        player->clips[i].clip         = NULL;
        player->clips[i].isPlaying    = false;
        player->clips[i].ownsClip     = false;
        player->clips[i].sampleOffset = 0;
        avdAudioPlayerConfigDefaults(&player->clips[i].config);
    }

    return true;
}

void avdAudioPlayerShutdown(AVD_Audio *audio, AVD_AudioPlayer *player)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(player != NULL);

    avdAudioPlayerStop(player);

    memset(player, 0, sizeof(AVD_AudioPlayer));
}

bool avdAudioPlayerConfigDefaults(AVD_AudioPlayerConfig *config)
{
    AVD_ASSERT(config != NULL);

    config->volume = 1.0f;
    config->pitch  = 1.0f;
    config->speed  = 1.0f;
    config->loop   = false;

    return true;
}

bool avdAudioPlayerPlaySync(AVD_AudioPlayer *player, AVD_AudioClip *clip, AVD_AudioPlayerConfig config)
{
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(player->audio != NULL);
    AVD_ASSERT(clip != NULL);

    AVD_CHECK(avdAudioPlayerPlayAsync(player, clip, false, config));

    // Wait until the clip has finished playing
    avdSleep(avdAudioClipDurationMs(clip) + 100);

    // update player to clean up finished clips
    avdAudioPlayerUpdate(player);

    return true;
}

bool avdAudioPlayerPlayAsync(AVD_AudioPlayer *player, AVD_AudioClip *clip, AVD_Bool ownClip, AVD_AudioPlayerConfig config)
{
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(player->audio != NULL);
    AVD_ASSERT(clip != NULL);

    AVD_Int32 freeSlot = -1;
    for (AVD_Size i = 0; i < AVD_AUDIO_PLAYER_MAX_CLIPS; i++) {
        if (!player->clips[i].isPlaying && player->clips[i].clip == NULL) {
            freeSlot = (AVD_Int32)i;
            break;
        }
    }

    if (freeSlot < 0) {
        AVD_LOG_ERROR("No free audio player slots available");
        if (ownClip) {
            avdAudioClipFree(clip);
        }
        return false;
    }

    AVD_AudiPlayerContext *ctx = &player->clips[freeSlot];
    ctx->clip                  = clip;
    ctx->config                = config;
    ctx->isPlaying             = true;
    ctx->ownsClip              = ownClip;
    ctx->sampleOffset          = 0;
    ctx->parent                = player;

    AVD_AudioStream *stream = &player->streams[freeSlot];
    bool success            = avdAudioOpenOutputStream(
        player->audio,
        stream,
        AVD_AUDIO_DEVICE_DEFAULT,
        clip->sampleRate,
        clip->stereo,
        256,
        __avdAudioPlayerCallback,
        ctx);

    if (!success) {
        AVD_LOG_ERROR("Failed to open audio output stream for async playback");
        ctx->isPlaying = false;
        if (ownClip) {
            avdAudioClipFree(clip);
        }
        ctx->clip = NULL;
        return false;
    }

    if (!avdAudioStartStream(player->audio, stream)) {
        AVD_LOG_ERROR("Failed to start audio stream for async playback");
        avdAudioCloseStream(player->audio, stream);
        ctx->isPlaying = false;
        if (ownClip) {
            avdAudioClipFree(clip);
        }
        ctx->clip = NULL;
        return false;
    }

    return true;
}

bool avdAudioPlayerStop(AVD_AudioPlayer *player)
{
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(player->audio != NULL);

    for (AVD_Size i = 0; i < AVD_AUDIO_PLAYER_MAX_CLIPS; i++) {
        AVD_AudiPlayerContext *ctx = &player->clips[i];

        if (ctx->isPlaying || ctx->clip != NULL) {
            AVD_AudioStream *stream = &player->streams[i];
            if (stream->stream) {
                avdAudioStopStream(player->audio, stream);
                avdAudioStreamDestroy(player->audio, stream);
                stream->stream = NULL;
            }

            if (ctx->ownsClip && ctx->clip) {
                avdAudioClipFree(ctx->clip);
            }

            ctx->clip         = NULL;
            ctx->isPlaying    = false;
            ctx->ownsClip     = false;
            ctx->sampleOffset = 0;
        }
    }

    return true;
}

bool avdAudioPlayerUpdate(AVD_AudioPlayer *player)
{
    AVD_ASSERT(player != NULL);
    AVD_ASSERT(player->audio != NULL);

    for (AVD_Size i = 0; i < AVD_AUDIO_PLAYER_MAX_CLIPS; i++) {
        AVD_AudiPlayerContext *ctx = &player->clips[i];

        if (!ctx->isPlaying && ctx->clip != NULL) {
            AVD_AudioStream *stream = &player->streams[i];
            if (stream->stream) {
                avdAudioStopStream(player->audio, stream);
                avdAudioStreamDestroy(player->audio, stream);
            }

            if (ctx->ownsClip) {
                avdAudioClipFree(ctx->clip);
            }

            ctx->clip         = NULL;
            ctx->ownsClip     = false;
            ctx->sampleOffset = 0;
        }
    }

    return true;
}
