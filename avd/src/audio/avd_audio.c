#include "audio/avd_audio.h"

#include "pico/picoAudio.h"

#include "AL/al.h"
#include "AL/alc.h"

#define AL_CALL(call)                                                      \
    do {                                                                   \
        call;                                                              \
        ALenum __alError = alGetError();                                   \
        if (__alError != AL_NO_ERROR) {                                    \
            AVD_LOG_ERROR("OpenAL error in %s: 0x%04X", #call, __alError); \
        }                                                                  \
    } while (0)

static bool __avdAudioBufferFromDecoder(picoAudioDecoder decoder, AVD_AudioBuffer *outBuffer)
{
    AVD_ASSERT(decoder != NULL);
    AVD_ASSERT(outBuffer != NULL);

    picoAudioInfo_t info = {0};
    if (picoAudioDecoderGetAudioInfo(decoder, &info) != PICO_AUDIO_RESULT_SUCCESS) {
        AVD_LOG_ERROR("Failed to get audio info from decoder");
        return false;
    }

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

    ALuint alBuffer = (ALuint)*outBuffer;
    if (alBuffer == 0) {
        AL_CALL(alGenBuffers(1, &alBuffer));
        *outBuffer = (AVD_AudioBuffer)alBuffer;
    }

    AL_CALL(alBufferData(
        alBuffer,
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

bool avdAudioLoadBufferFromFile(AVD_Audio *audio, const char *filename, AVD_AudioBuffer *outBuffer)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(filename != NULL);
    AVD_ASSERT(outBuffer != NULL);

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

    bool result = __avdAudioBufferFromDecoder(decoder, outBuffer);
    picoAudioDecoderDestroy(decoder);
    return result;
}

bool avdAudioLoadBufferFromMemory(AVD_Audio *audio, const void *data, size_t size, AVD_AudioBuffer *outBuffer)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(data != NULL);
    AVD_ASSERT(outBuffer != NULL);

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

    bool result = __avdAudioBufferFromDecoder(decoder, outBuffer);
    picoAudioDecoderDestroy(decoder);
    return result;
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

    bool isPlaying = false;
    (void)avdAudioSourceIsPlaying(audio, source, &isPlaying);
    if (isPlaying) {
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

bool avdAudioSourceIsPlaying(AVD_Audio *audio, AVD_AudioSource source, bool *outIsPlaying)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(outIsPlaying != NULL);

    ALint state;
    AL_CALL(alGetSourcei((ALuint)source, AL_SOURCE_STATE, &state));
    *outIsPlaying = (state == AL_PLAYING);
    return true;
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