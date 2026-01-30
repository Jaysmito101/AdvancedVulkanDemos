#include "audio/avd_audio_core.h"

#include "pico/picoAudio.h"

#include "AL/al.h"
#include "AL/alc.h"
#include <stdbool.h>

static bool __avdAudioBufferFromDecoder(picoAudioDecoder decoder, AVD_AudioBuffer buffer)
{
    AVD_ASSERT(decoder != NULL);

    picoAudioInfo_t info = {0};
    if (picoAudioDecoderGetAudioInfo(decoder, &info) != PICO_AUDIO_RESULT_SUCCESS) {
        AVD_LOG_ERROR("Failed to get audio info from decoder");
        return false;
    }

    AVD_LOG_DEBUG("Loading audio buffer: %u Hz, %u channels, %u bits per sample, %llu total samples. duration: %.2f seconds",
                  info.sampleRate,
                  info.channelCount,
                  info.bitsPerSample,
                  (unsigned long long)info.totalSamples,
                  (double)info.durationSeconds);
    AVD_Size bytesPerSample = (info.bitsPerSample / 8) * info.channelCount;
    AVD_Size dataSize       = info.totalSamples * bytesPerSample;
    AVD_UInt8 *pcmData      = (AVD_UInt8 *)AVD_MALLOC(dataSize);
    if (pcmData == NULL) {
        AVD_LOG_ERROR("Failed to allocate memory for PCM data");
        AVD_FREE(pcmData);
        return false;
    }

    AVD_Size samplesRead = 0;
    if (picoAudioDecoderDecode(
            decoder,
            (AVD_Int16 *)pcmData,
            dataSize / bytesPerSample,
            &samplesRead) != PICO_AUDIO_RESULT_SUCCESS) {
        AVD_LOG_ERROR("Failed to read samples from decoder");
        AVD_FREE(pcmData);
        return false;
    }

    AL_CALL(alBufferData(
        (ALuint)buffer,
        (info.channelCount == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
        pcmData,
        (ALsizei)dataSize,
        (ALsizei)info.sampleRate));

    AVD_FREE(pcmData);

    return true;
}

bool avdAudioInit(AVD_Audio *audio)
{
    AVD_ASSERT(audio != NULL);

    memset(audio, 0, sizeof(AVD_Audio));

    audio->device = alcOpenDevice(NULL);
    if (audio->device == NULL) {
        AVD_LOG_ERROR("Failed to open OpenAL device");
        avdAudioShutdown(audio);
        return false;
    }

    audio->context = alcCreateContext((ALCdevice *)audio->device, NULL);
    if (audio->context == NULL) {
        AVD_LOG_ERROR("Failed to create OpenAL context");
        avdAudioShutdown(audio);
        return false;
    }

    if (!alcMakeContextCurrent((ALCcontext *)audio->context)) {
        AVD_LOG_ERROR("Failed to make OpenAL context current");
        avdAudioShutdown(audio);
        return false;
    }

    return true;
}

void avdAudioShutdown(AVD_Audio *audio)
{
    AVD_ASSERT(audio != NULL);

    if (audio->context) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext((ALCcontext *)audio->context);
        audio->context = NULL;
    }

    if (audio->device) {
        alcCloseDevice((ALCdevice *)audio->device);
        audio->device = NULL;
    }
}

bool avdAudioBufferCreate(AVD_Audio *audio, AVD_AudioBuffer *outBuffer)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(outBuffer != NULL);

    AL_CALL(alGenBuffers(1, (ALuint *)outBuffer));
    return true;
}

bool avdAudioLoadBufferFromFile(AVD_Audio *audio, const char *filename, AVD_AudioBuffer buffer)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(filename != NULL);

    picoAudioDecoder decoder = picoAudioDecoderCreate();
    if (decoder == NULL) {
        AVD_LOG_ERROR("Failed to create audio decoder");
        return false;
    }

    if (picoAudioDecoderOpenFile(decoder, filename) != PICO_AUDIO_RESULT_SUCCESS) {
        AVD_LOG_ERROR("Failed to open audio file: %s", filename);
        picoAudioDecoderDestroy(decoder);
        return false;
    }

    bool result = __avdAudioBufferFromDecoder(decoder, buffer);
    picoAudioDecoderDestroy(decoder);
    return result;
}

bool avdAudioLoadBufferFromMemory(AVD_Audio *audio, const void *data, size_t size, AVD_AudioBuffer buffer)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(data != NULL);

    picoAudioDecoder decoder = picoAudioDecoderCreate();
    if (decoder == NULL) {
        AVD_LOG_ERROR("Failed to create audio decoder");
        return false;
    }

    if (picoAudioDecoderOpenBuffer(decoder, data, size) != PICO_AUDIO_RESULT_SUCCESS) {
        AVD_LOG_ERROR("Failed to open audio data from memory");
        picoAudioDecoderDestroy(decoder);
        return false;
    }

    bool result = __avdAudioBufferFromDecoder(decoder, buffer);
    picoAudioDecoderDestroy(decoder);
    return result;
}

bool avdAudioLoadEmptyBuffer(AVD_Audio *audio, AVD_AudioBuffer buffer, AVD_UInt32 sampleRate, AVD_UInt32 channels)
{
    // just load a simple silent buffer
    AVD_ASSERT(audio != NULL);

    static int16_t slientSamples[16] = {0};
    AL_CALL(alBufferData(
        (ALuint)buffer,
        (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
        slientSamples,
        sizeof(slientSamples),
        (ALsizei)sampleRate));

    return true;
}

void avdAudioBufferDestroy(AVD_Audio *audio, AVD_AudioBuffer buffer)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alDeleteBuffers(1, (ALuint *)&buffer));
}

bool avdAudioSourceCreate(AVD_Audio *audio, AVD_AudioSource *outSource)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(outSource != NULL);

    AL_CALL(alGenSources(1, (ALuint *)outSource));
    AVD_CHECK(avdAudioSourceSetupDefaults(audio, *outSource));
    return true;
}

void avdAudioSourceDestroy(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    if (avdAudioSourceIsPlaying(audio, source)) {
        (void)avdAudioSourceStop(audio, source);
    }
    (void)avdAudioSourceUnlinkBuffer(audio, source);

    AL_CALL(alDeleteSources(1, (ALuint *)&source));
}

bool avdAudioSourceLinkBuffer(AVD_Audio *audio, AVD_AudioSource source, AVD_AudioBuffer buffer)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSourcei((ALuint)source, AL_BUFFER, (ALint)buffer));
    return true;
}

bool avdAudioSourceUnlinkBuffer(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSourcei((ALuint)source, AL_BUFFER, 0));
    return true;
}

bool avdAudioSourcePlay(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSourcePlay((ALuint)source));
    return true;
}

bool avdAudioSourcePause(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSourcePause((ALuint)source));
    return true;
}

bool avdAudioSourceStop(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSourceStop((ALuint)source));
    return true;
}

bool avdAudioSourceSetLooping(AVD_Audio *audio, AVD_AudioSource source, bool looping)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSourcei((ALuint)source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE));
    return true;
}

bool avdAudioSourceSetGain(AVD_Audio *audio, AVD_AudioSource source, float gain)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSourcef((ALuint)source, AL_GAIN, gain));
    return true;
}

bool avdAudioSourceIsPlaying(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    ALint state;
    AL_CALL(alGetSourcei((ALuint)source, AL_SOURCE_STATE, &state));
    return (state == AL_PLAYING);
}

bool avdAudioSourceIsPaused(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    ALint state;
    AL_CALL(alGetSourcei((ALuint)source, AL_SOURCE_STATE, &state));
    return (state == AL_PAUSED);
}

bool avdAudioSourceIsStopped(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    ALint state;
    AL_CALL(alGetSourcei((ALuint)source, AL_SOURCE_STATE, &state));
    return (state == AL_STOPPED);
}

bool avdAudioSourceSetPosition(AVD_Audio *audio, AVD_AudioSource source, float x, float y, float z)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSource3f((ALuint)source, AL_POSITION, x, y, z));
    return true;
}

bool avdAudioSourceSetVelocity(AVD_Audio *audio, AVD_AudioSource source, float x, float y, float z)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSource3f((ALuint)source, AL_VELOCITY, x, y, z));
    return true;
}

bool avdAudioSourceSetupDefaults(AVD_Audio *audio, AVD_AudioSource source)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alSourcef((ALuint)source, AL_PITCH, 1.0f));
    AL_CALL(alSourcef((ALuint)source, AL_GAIN, 1.0f));
    AL_CALL(alSource3f((ALuint)source, AL_POSITION, 0.0f, 0.0f, 0.0f));
    AL_CALL(alSource3f((ALuint)source, AL_VELOCITY, 0.0f, 0.0f, 0.0f));
    AL_CALL(alSourcei((ALuint)source, AL_LOOPING, AL_FALSE));
    return true;
}

bool avdAudioSourceEnqueueBuffers(AVD_Audio *audio, AVD_AudioSource source, AVD_AudioBuffer *buffers, AVD_Size bufferCount)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(buffers != NULL);
    AVD_ASSERT(bufferCount > 0);

    AL_CALL(alSourceQueueBuffers((ALuint)source, (ALsizei)bufferCount, (ALuint *)buffers));
    return true;
}

bool avdAudioSourceUnqueueBuffers(AVD_Audio *audio, AVD_AudioSource source, AVD_AudioBuffer *buffers, AVD_Size bufferCount)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(buffers != NULL);
    AVD_ASSERT(bufferCount > 0);

    AL_CALL(alSourceUnqueueBuffers((ALuint)source, (ALsizei)bufferCount, (ALuint *)buffers));
    return true;
}

bool avdAudioSourceGetProcessedBufferCount(AVD_Audio *audio, AVD_AudioSource source, AVD_Size *outCount)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(outCount != NULL);

    ALint count = 0;
    AL_CALL(alGetSourcei((ALuint)source, AL_BUFFERS_PROCESSED, &count));
    *outCount = (AVD_Size)count;
    return true;
}

bool avdAudioSourceGetQueuedBufferCount(AVD_Audio *audio, AVD_AudioSource source, AVD_Size *outCount)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(outCount != NULL);

    ALint count = 0;
    AL_CALL(alGetSourcei((ALuint)source, AL_BUFFERS_QUEUED, &count));
    *outCount = (AVD_Size)count;
    return true;
}

bool avdAudioListenerSetDirection(AVD_Audio *audio, float x, float y, float z)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alListener3f(AL_ORIENTATION, x, y, z));
    return true;
}

bool avdAudioListenerSetPosition(AVD_Audio *audio, float x, float y, float z)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alListener3f(AL_POSITION, x, y, z));
    return true;
}

bool avdAudioListenerSetVelocity(AVD_Audio *audio, float x, float y, float z)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alListener3f(AL_VELOCITY, x, y, z));
    return true;
}

bool avdAudioListenerSetOrientation(AVD_Audio *audio, float atX, float atY, float atZ, float upX, float upY, float upZ)
{
    AVD_ASSERT(audio != NULL);

    float orientation[6] = {atX, atY, atZ, upX, upY, upZ};
    AL_CALL(alListenerfv(AL_ORIENTATION, orientation));
    return true;
}

bool avdAudioListenerSetupDefaults(AVD_Audio *audio)
{
    AVD_ASSERT(audio != NULL);

    AL_CALL(alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f));
    AL_CALL(alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f));
    float orientation[6] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
    AL_CALL(alListenerfv(AL_ORIENTATION, orientation));
    return true;
}