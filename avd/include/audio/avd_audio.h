#ifndef AVD_AUDIO_H
#define AVD_AUDIO_H

#include "core/avd_core.h"
#include "core/avd_types.h"

typedef AVD_UInt32 AVD_AudioBuffer;
typedef AVD_UInt32 AVD_AudioSource;

typedef struct {
    void *device;
    void *context;
} AVD_Audio;

bool avdAudioInit(AVD_Audio *audio);
void avdAudioShutdown(AVD_Audio *audio);

bool avdAudioLoadBufferFromFile(AVD_Audio *audio, const char *filename, AVD_AudioBuffer *outBuffer);
bool avdAudioLoadBufferFromMemory(AVD_Audio *audio, const void *data, size_t size, AVD_AudioBuffer *outBuffer);
void avdAudioBufferDestroy(AVD_Audio *audio, AVD_AudioBuffer buffer);

bool avdAudioSourceCreate(AVD_Audio *audio, AVD_AudioSource *outSource);
void avdAudioSourceDestroy(AVD_Audio *audio, AVD_AudioSource source);
bool avdAudioSourceLinkBuffer(AVD_Audio *audio, AVD_AudioSource source, AVD_AudioBuffer buffer);
bool avdAudioSourceUnlinkBuffer(AVD_Audio *audio, AVD_AudioSource source);
bool avdAudioSourcePlay(AVD_Audio *audio, AVD_AudioSource source);
bool avdAudioSourceStop(AVD_Audio *audio, AVD_AudioSource source);
bool avdAudioSourceSetLooping(AVD_Audio *audio, AVD_AudioSource source, bool looping);
bool avdAudioSourceSetGain(AVD_Audio *audio, AVD_AudioSource source, float gain);
bool avdAudioSourceIsPlaying(AVD_Audio *audio, AVD_AudioSource source, bool *outIsPlaying);
bool avdAudioSourceSetPosition(AVD_Audio *audio, AVD_AudioSource source, float x, float y, float z);
bool avdAudioSourceSetVelocity(AVD_Audio *audio, AVD_AudioSource source, float x, float y, float z);
bool avdAudioSourceSetupDefaults(AVD_Audio *audio, AVD_AudioSource source);

bool avdAudioListenerSetDirection(AVD_Audio *audio, float x, float y, float z);
bool avdAudioListenerSetPosition(AVD_Audio *audio, float x, float y, float z);
bool avdAudioListenerSetVelocity(AVD_Audio *audio, float x, float y, float z);
bool avdAudioListenerSetOrientation(AVD_Audio *audio, float atX, float atY, float atZ, float upX, float upY, float upZ);
bool avdAudioListenerSetupDefaults(AVD_Audio *audio);

#endif // AVD_AUDIO_H