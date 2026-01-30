#include "audio/avd_audio_clip.h"
#include "pico/picoAudio.h"

static bool __avdAudioClipFromDecoder(picoAudioDecoder decoder, AVD_AudioClip *outAudioData)
{
    AVD_ASSERT(decoder != NULL);
    AVD_ASSERT(outAudioData != NULL);

    memset(outAudioData, 0, sizeof(AVD_AudioClip));

    picoAudioInfo_t audioInfo = {0};
    picoAudioResult result    = picoAudioDecoderGetAudioInfo(decoder, &audioInfo);
    if (result != PICO_AUDIO_RESULT_SUCCESS) {
        AVD_LOG_ERROR("Failed to get audio info from decoder");
        picoAudioDecoderDestroy(decoder);
        return false;
    }

    outAudioData->sampleRate    = audioInfo.sampleRate;
    outAudioData->channels      = audioInfo.channelCount;
    outAudioData->bitsPerSample = audioInfo.bitsPerSample;
    outAudioData->stereo        = (audioInfo.channelCount == 2) ? true : false;

    size_t totalSamples = audioInfo.totalSamples * audioInfo.channelCount;
    size_t totalBytes   = totalSamples * (audioInfo.bitsPerSample / 8);

    outAudioData->samples     = PICO_MALLOC(totalBytes);
    outAudioData->sampleCount = totalSamples;

    if (!outAudioData->samples) {
        AVD_LOG_ERROR("Failed to allocate memory for audio samples");
        picoAudioDecoderDestroy(decoder);
        return false;
    }

    size_t framesRead      = 0;
    size_t totalFramesRead = 0;
    uint8_t *bufferPtr     = (uint8_t *)outAudioData->samples;

    picoAudioDecoderDecode(
        decoder,
        bufferPtr,
        totalSamples,
        &framesRead);

    outAudioData->sampleCount = framesRead;

    picoAudioDecoderDestroy(decoder);
    return true;
}

bool avdAudioClipFromFile(const char *filePath, AVD_AudioClip *outAudioData)
{
    picoAudioDecoder decoder = picoAudioDecoderCreate();
    if (!decoder) {
        AVD_LOG_ERROR("Failed to create audio decoder");
        return false;
    }

    picoAudioResult result = picoAudioDecoderOpenFile(decoder, filePath);
    if (result != PICO_AUDIO_RESULT_SUCCESS) {
        AVD_LOG_ERROR("Failed to open audio file: %s", filePath);
        picoAudioDecoderDestroy(decoder);
        return false;
    }

    return __avdAudioClipFromDecoder(decoder, outAudioData);
}

bool avdAudioClipFromMemory(const void *data, AVD_Size dataSize, AVD_AudioClip *outAudioData)
{
    picoAudioDecoder decoder = picoAudioDecoderCreate();
    if (!decoder) {
        AVD_LOG_ERROR("Failed to create audio decoder");
        return false;
    }

    picoAudioResult result = picoAudioDecoderOpenBuffer(decoder, (const uint8_t *)data, dataSize);
    if (result != PICO_AUDIO_RESULT_SUCCESS) {
        AVD_LOG_ERROR("Failed to open audio buffer");
        picoAudioDecoderDestroy(decoder);
        return false;
    }

    return __avdAudioClipFromDecoder(decoder, outAudioData);
}

bool avdAudioClipFromRawPCM(
    const void *data,
    AVD_Size dataSize,
    AVD_UInt32 sampleRate,
    AVD_UInt32 channels,
    AVD_UInt32 bitsPerSample,
    AVD_AudioClip *outAudioData)
{
    AVD_ASSERT(data != NULL);
    AVD_ASSERT(dataSize > 0);
    AVD_ASSERT(sampleRate > 0);
    AVD_ASSERT(channels > 0);
    AVD_ASSERT(bitsPerSample > 0);
    AVD_ASSERT(outAudioData != NULL);

    memset(outAudioData, 0, sizeof(AVD_AudioClip));

    outAudioData->samples       = PICO_MALLOC(dataSize);
    outAudioData->sampleCount   = dataSize / (bitsPerSample / 8);
    outAudioData->sampleRate    = sampleRate;
    outAudioData->channels      = channels;
    outAudioData->bitsPerSample = bitsPerSample;
    outAudioData->stereo        = (channels == 2) ? true : false;

    if (!outAudioData->samples) {
        AVD_LOG_ERROR("Failed to allocate memory for raw PCM audio data");
        return false;
    }

    memcpy(outAudioData->samples, data, dataSize);
    return true;
}

bool avdAudioClipSilent(
    AVD_UInt32 durationMs,
    AVD_UInt32 sampleRate,
    AVD_UInt32 channels,
    AVD_UInt32 bitsPerSample,
    AVD_AudioClip *outAudioData)
{
    AVD_ASSERT(durationMs > 0);
    AVD_ASSERT(sampleRate > 0);
    AVD_ASSERT(channels > 0);
    AVD_ASSERT(bitsPerSample > 0);
    AVD_ASSERT(outAudioData != NULL);

    memset(outAudioData, 0, sizeof(AVD_AudioClip));

    AVD_Size totalSamples = (AVD_Size)((durationMs / 1000.0) * sampleRate * channels);
    AVD_Size totalBytes   = totalSamples * (bitsPerSample / 8);

    outAudioData->samples       = PICO_MALLOC(totalBytes);
    outAudioData->sampleCount   = totalSamples;
    outAudioData->sampleRate    = sampleRate;
    outAudioData->channels      = channels;
    outAudioData->bitsPerSample = bitsPerSample;
    outAudioData->stereo        = (channels == 2) ? true : false;

    if (!outAudioData->samples) {
        AVD_LOG_ERROR("Failed to allocate memory for silent audio data");
        return false;
    }

    memset(outAudioData->samples, 0, totalBytes);
    return true;
}

bool avdAudioClipFree(AVD_AudioClip *audioData)
{
    AVD_ASSERT(audioData != NULL);

    if (audioData->samples) {
        PICO_FREE(audioData->samples);
        audioData->samples     = NULL;
        audioData->sampleCount = 0;
    }

    return true;
}

AVD_Float avdAudioClipDurationMs(const AVD_AudioClip *audioData)
{
    AVD_ASSERT(audioData != NULL);
    AVD_ASSERT(audioData->sampleRate > 0);
    AVD_ASSERT(audioData->channels > 0);

    AVD_Size totalFrames = audioData->sampleCount / audioData->channels;
    return ((AVD_Float)totalFrames / (AVD_Float)audioData->sampleRate) * 1000.0f;
}

bool avdAudioClipSampleAtIndex(
    const AVD_AudioClip *audioData,
    AVD_Size index,
    AVD_Float *outSample)
{
    AVD_ASSERT(audioData != NULL);
    AVD_ASSERT(outSample != NULL);
    AVD_ASSERT(index < audioData->sampleCount);
    AVD_ASSERT(audioData->bitsPerSample == 16 || audioData->bitsPerSample == 32);

    if (audioData->bitsPerSample == 16) {
        int16_t *samples16 = (int16_t *)audioData->samples;
        *outSample         = (AVD_Float)samples16[index] / 32768.0f;
    } else if (audioData->bitsPerSample == 32) { //  maybe it could be float too?
        int32_t *samples32 = (int32_t *)audioData->samples;
        *outSample         = (AVD_Float)samples32[index] / 2147483648.0f;
    } else {
        AVD_LOG_ERROR("Unsupported bits per sample: %u", audioData->bitsPerSample);
        return false;
    }

    return true;
}

bool avdAudioClipSampleAtTimeMs(
    const AVD_AudioClip *audioData,
    AVD_Float timeMs,
    AVD_Float *outSample)
{
    AVD_ASSERT(audioData != NULL);
    AVD_ASSERT(outSample != NULL);
    AVD_ASSERT(audioData->sampleRate > 0);
    AVD_ASSERT(audioData->channels > 0);

    AVD_Size totalFrames      = audioData->sampleCount / audioData->channels;
    AVD_Float totalDurationMs = ((AVD_Float)totalFrames / (AVD_Float)audioData->sampleRate) * 1000.0f;

    if (timeMs < 0.0f || timeMs > totalDurationMs) {
        AVD_LOG_ERROR("Time %f ms is out of bounds (0 - %f ms)", timeMs, totalDurationMs);
        return false;
    }

    AVD_Size targetFrameIndex  = (AVD_Size)((timeMs / 1000.0f) * (AVD_Float)audioData->sampleRate);
    AVD_Size targetSampleIndex = targetFrameIndex * audioData->channels;

    return avdAudioClipSampleAtIndex(audioData, targetSampleIndex, outSample);
}