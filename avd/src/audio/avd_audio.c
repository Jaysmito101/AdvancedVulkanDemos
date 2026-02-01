#include "audio/avd_audio_core.h"

#include "pico/picoAudio.h"
#include "portaudio.h"

static bool __avdAudioPaAudioDeviceToAVDAudioDevice(
    const PaDeviceInfo *paDeviceInfo,
    AVD_Size index,
    AVD_AudioDevice *outDevice)
{
    AVD_ASSERT(paDeviceInfo != NULL);
    AVD_ASSERT(outDevice != NULL);

    strncpy(outDevice->name, paDeviceInfo->name, sizeof(outDevice->name) - 1);
    outDevice->name[sizeof(outDevice->name) - 1] = '\0';

    outDevice->maxInputChannels         = (AVD_UInt32)paDeviceInfo->maxInputChannels;
    outDevice->maxOutputChannels        = (AVD_UInt32)paDeviceInfo->maxOutputChannels;
    outDevice->defaultLowInputLatency   = (AVD_Float)paDeviceInfo->defaultLowInputLatency;
    outDevice->defaultLowOutputLatency  = (AVD_Float)paDeviceInfo->defaultLowOutputLatency;
    outDevice->defaultHighInputLatency  = (AVD_Float)paDeviceInfo->defaultHighInputLatency;
    outDevice->defaultHighOutputLatency = (AVD_Float)paDeviceInfo->defaultHighOutputLatency;
    outDevice->defaultSampleRate        = (AVD_Float)paDeviceInfo->defaultSampleRate;
    outDevice->index                    = index;

    return true;
}

static int __avdAudioPaAudioCallback(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    (void)timeInfo;
    (void)statusFlags;

    AVD_AudioStream *audioStream = (AVD_AudioStream *)userData;
    if (audioStream && audioStream->callback) {
        return audioStream->callback(inputBuffer, outputBuffer, (AVD_Size)framesPerBuffer, statusFlags, audioStream->userData) ? paContinue : paAbort;
    }

    return paContinue;
}

bool avdAudioInit(AVD_Audio *audio)
{
    AVD_ASSERT(audio != NULL);
    memset(audio, 0, sizeof(AVD_Audio));

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        AVD_LOG_ERROR("Failed to initialize PortAudio: %s", Pa_GetErrorText(err));
        return false;
    }

    PaDeviceIndex defaultInputDeviceIndex       = Pa_GetDefaultInputDevice();
    PaDeviceIndex defaultOutputDeviceIndex      = Pa_GetDefaultOutputDevice();
    const PaDeviceInfo *defaultInputDeviceInfo  = Pa_GetDeviceInfo(defaultInputDeviceIndex);
    const PaDeviceInfo *defaultOutputDeviceInfo = Pa_GetDeviceInfo(defaultOutputDeviceIndex);
    if (defaultInputDeviceInfo) {
        AVD_LOG_INFO("Default Audio Input Device: %s", defaultInputDeviceInfo->name);
    }
    if (defaultOutputDeviceInfo) {
        AVD_LOG_INFO("Default Audio Output Device: %s", defaultOutputDeviceInfo->name);
    }

    return true;
}

void avdAudioShutdown(AVD_Audio *audio)
{
    AVD_ASSERT(audio != NULL);

    Pa_Terminate();
    memset(audio, 0, sizeof(AVD_Audio));
}

bool avdAudioGetDevices(AVD_Audio *audio, AVD_AudioDevice *devices, AVD_Size *deviceCount, AVD_Size maxDevices)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(deviceCount != NULL);

    PaDeviceIndex paDeviceCount = Pa_GetDeviceCount();
    if (paDeviceCount < 0) {
        AVD_LOG_ERROR("Failed to get PortAudio device count: %s", Pa_GetErrorText((PaError)paDeviceCount));
        return false;
    }

    AVD_Size count = (AVD_Size)paDeviceCount;
    if (maxDevices < count) {
        count = maxDevices;
    }

    if (devices) {
        for (AVD_Size i = 0; i < count; i++) {
            const PaDeviceInfo *paDeviceInfo = Pa_GetDeviceInfo((PaDeviceIndex)i);
            __avdAudioPaAudioDeviceToAVDAudioDevice(paDeviceInfo, i, &devices[i]);
        }
    }

    *deviceCount = count;
    return true;
}

bool avdAudioGetDefaultOutputDevice(AVD_Audio *audio, AVD_AudioDevice *device)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(device != NULL);

    PaDeviceIndex defaultOutputDeviceIndex = Pa_GetDefaultOutputDevice();
    if (defaultOutputDeviceIndex == paNoDevice) {
        AVD_LOG_ERROR("No default output device found");
        return false;
    }

    const PaDeviceInfo *paDeviceInfo = Pa_GetDeviceInfo(defaultOutputDeviceIndex);
    if (!paDeviceInfo) {
        AVD_LOG_ERROR("Failed to get default output device info");
        return false;
    }

    __avdAudioPaAudioDeviceToAVDAudioDevice(paDeviceInfo, (AVD_Size)defaultOutputDeviceIndex, device);
    return true;
}

bool avdAudioGetDefaultInputDevice(AVD_Audio *audio, AVD_AudioDevice *device)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(device != NULL);

    PaDeviceIndex defaultInputDeviceIndex = Pa_GetDefaultInputDevice();
    if (defaultInputDeviceIndex == paNoDevice) {
        AVD_LOG_ERROR("No default input device found");
        return false;
    }

    const PaDeviceInfo *paDeviceInfo = Pa_GetDeviceInfo(defaultInputDeviceIndex);
    if (!paDeviceInfo) {
        AVD_LOG_ERROR("Failed to get default input device info");
        return false;
    }

    __avdAudioPaAudioDeviceToAVDAudioDevice(paDeviceInfo, (AVD_Size)defaultInputDeviceIndex, device);
    return true;
}

bool avdAudioOpenOutputStream(
    AVD_Audio *audio,
    AVD_AudioStream *stream,
    AVD_UInt32 outputDeviceIndex,
    AVD_UInt32 sampleRate,
    AVD_Bool stereo,
    AVD_UInt32 framesPerBuffer,
    AVD_AudioCallback *callback,
    void *userData)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(stream != NULL);

    memset(stream, 0, sizeof(AVD_AudioStream));

    stream->userData = userData;
    stream->audio    = audio;
    stream->callback = callback;

    PaDeviceIndex deviceIndex = (outputDeviceIndex == AVD_AUDIO_DEVICE_DEFAULT) ? Pa_GetDefaultOutputDevice() : (PaDeviceIndex)outputDeviceIndex;

    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if (!deviceInfo) {
        AVD_LOG_ERROR("Failed to get device info for device index %d", deviceIndex);
        return false;
    }

    PaStreamParameters outputParameters        = {0};
    outputParameters.device                    = deviceIndex;
    outputParameters.channelCount              = stereo ? 2 : 1;
    outputParameters.sampleFormat              = paFloat32;
    outputParameters.suggestedLatency          = deviceInfo->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        (PaStream **)&stream->stream,
        NULL,
        &outputParameters,
        (double)sampleRate,
        framesPerBuffer,
        paClipOff,
        __avdAudioPaAudioCallback,
        stream);
    if (err != paNoError) {
        AVD_LOG_ERROR("Failed to open audio output stream: %s", Pa_GetErrorText(err));
        return false;
    }

    return true;
}

bool avdAudioOpenInputStream(
    AVD_Audio *audio,
    AVD_AudioStream *stream,
    AVD_UInt32 inputDeviceIndex,
    AVD_UInt32 sampleRate,
    AVD_Bool stereo,
    AVD_UInt32 framesPerBuffer,
    AVD_AudioCallback *callback,
    void *userData)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(stream != NULL);

    memset(stream, 0, sizeof(AVD_AudioStream));

    stream->userData = userData;
    stream->audio    = audio;
    stream->callback = callback;

    PaDeviceIndex deviceIndex = (inputDeviceIndex == AVD_AUDIO_DEVICE_DEFAULT) ? Pa_GetDefaultInputDevice() : (PaDeviceIndex)inputDeviceIndex;

    PaStreamParameters inputParameters        = {0};
    inputParameters.device                    = deviceIndex;
    inputParameters.channelCount              = stereo ? 2 : 1;
    inputParameters.sampleFormat              = paFloat32;
    inputParameters.suggestedLatency          = Pa_GetDeviceInfo(deviceIndex)->defaultHighInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        (PaStream **)&stream->stream,
        &inputParameters,
        NULL,
        (double)sampleRate,
        framesPerBuffer,
        paClipOff,
        __avdAudioPaAudioCallback,
        stream);
    if (err != paNoError) {
        AVD_LOG_ERROR("Failed to open audio input stream: %s", Pa_GetErrorText(err));
        return false;
    }

    return true;
}

bool avdAudioStartStream(AVD_Audio *audio, AVD_AudioStream *stream)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(stream != NULL);

    PaError err = Pa_StartStream((PaStream *)stream->stream);
    if (err != paNoError) {
        AVD_LOG_ERROR("Failed to start audio stream: %s", Pa_GetErrorText(err));
        return false;
    }

    return true;
}

bool avdAudioStopStream(AVD_Audio *audio, AVD_AudioStream *stream)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(stream != NULL);

    PaError err = Pa_StopStream((PaStream *)stream->stream);
    if (err != paNoError) {
        AVD_LOG_ERROR("Failed to stop audio stream: %s", Pa_GetErrorText(err));
        return false;
    }

    return true;
}

bool avdAudioAbortStream(AVD_Audio *audio, AVD_AudioStream *stream)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(stream != NULL);

    PaError err = Pa_AbortStream((PaStream *)stream->stream);
    if (err != paNoError) {
        AVD_LOG_ERROR("Failed to abort audio stream: %s", Pa_GetErrorText(err));
        return false;
    }

    return true;
}

bool avdAudioCloseStream(AVD_Audio *audio, AVD_AudioStream *stream)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(stream != NULL);

    PaError err = Pa_CloseStream((PaStream *)stream->stream);
    if (err != paNoError) {
        AVD_LOG_ERROR("Failed to close audio stream: %s", Pa_GetErrorText(err));
        return false;
    }

    memset(stream, 0, sizeof(AVD_AudioStream));
    return true;
}

void avdAudioStreamDestroy(AVD_Audio *audio, AVD_AudioStream *stream)
{
    AVD_ASSERT(audio != NULL);
    AVD_ASSERT(stream != NULL);

    if (stream->stream) {
        avdAudioCloseStream(audio, stream);
    }
}