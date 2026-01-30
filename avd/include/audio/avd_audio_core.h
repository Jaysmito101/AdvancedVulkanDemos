#ifndef AVD_AUDIO_CORE_H
#define AVD_AUDIO_CORE_H

#include "core/avd_core.h"
#include "core/avd_types.h"


#define AVD_AUDIO_DEVICE_DEFAULT UINT32_MAX

typedef struct {
    void *context;
} AVD_Audio;

typedef struct {
    char name[256];
    AVD_UInt32 maxInputChannels;
    AVD_UInt32 maxOutputChannels;

    AVD_Float defaultLowInputLatency;
    AVD_Float defaultLowOutputLatency;

    AVD_Float defaultHighInputLatency;
    AVD_Float defaultHighOutputLatency;

    AVD_Float defaultSampleRate;

    AVD_Size index;
} AVD_AudioDevice;

typedef bool (AVD_AudioCallback)(
    const void *inputBuffer,
    void *outputBuffer,
    AVD_Size framesPerBuffer,
    AVD_UInt32 statusFlags,
    void *userData);

typedef struct {
    AVD_Audio* audio;
    void* stream;
    void* userData;
    AVD_AudioCallback* callback;
    AVD_Size device;
} AVD_AudioStream;


bool avdAudioInit(AVD_Audio *audio);
void avdAudioShutdown(AVD_Audio *audio);

bool avdAudioGetDevices(AVD_Audio *audio, AVD_AudioDevice *devices, AVD_Size *deviceCount, AVD_Size maxDevices);
bool avdAudioGetDefaultOutputDevice(AVD_Audio *audio, AVD_AudioDevice *device);
bool avdAudioGetDefaultInputDevice(AVD_Audio *audio, AVD_AudioDevice *device);

bool avdAudioOpenOutputStream(
    AVD_Audio *audio,
    AVD_AudioStream *stream,
    AVD_UInt32 outputDeviceIndex,
    AVD_UInt32 sampleRate,
    AVD_Bool stereo,
    AVD_UInt32 framesPerBuffer,
    AVD_AudioCallback *callback,
    void *userData);
bool avdAudioOpenInputStream(
    AVD_Audio *audio,
    AVD_AudioStream *stream,
    AVD_UInt32 inputDeviceIndex,
    AVD_UInt32 sampleRate,
    AVD_Bool stereo,
    AVD_UInt32 framesPerBuffer,
    AVD_AudioCallback *callback,
    void *userData);

bool avdAudioStartStream(AVD_Audio *audio, AVD_AudioStream *stream);
bool avdAudioStopStream(AVD_Audio *audio, AVD_AudioStream *stream);
bool avdAudioCloseStream(AVD_Audio *audio, AVD_AudioStream *stream);
void avdAudioStreamDestroy(AVD_Audio *audio, AVD_AudioStream *stream);


#endif // AVD_AUDIO_CORE_H