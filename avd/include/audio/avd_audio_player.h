#ifndef AVD_AUDIO_PLAYER_H
#define AVD_AUDIO_PLAYER_H

#include "audio/avd_audio_core.h"
#include "avd_audio_clip.h"

#define AVD_AUDIO_PLAYER_MAX_CLIPS 16

typedef struct AVD_AudioPlayer AVD_AudioPlayer;

typedef struct {
    AVD_Float volume; // Volume level (0.0 to 1.0)
    AVD_Float pitch;  // Pitch adjustment (1.0 = normal pitch)
    AVD_Float speed;  // Playback speed (1.0 = normal speed)
    AVD_Bool loop;    // Whether to loop the audio clip
} AVD_AudioPlayerConfig;

typedef struct {
    AVD_AudioPlayer *parent;
    AVD_AudioClip *clip;
    AVD_AudioPlayerConfig config;
    bool isPlaying;
    bool ownsClip;
    AVD_Size sampleOffset;
} AVD_AudiPlayerContext;

struct AVD_AudioPlayer {
    bool initialized;

    AVD_Audio *audio;

    AVD_Float globalVolume;
    AVD_AudiPlayerContext clips[AVD_AUDIO_PLAYER_MAX_CLIPS];
    AVD_AudioStream streams[AVD_AUDIO_PLAYER_MAX_CLIPS];
};

bool avdAudioPlayerInit(AVD_Audio *audio, AVD_AudioPlayer *player);
void avdAudioPlayerShutdown(AVD_AudioPlayer *player);

bool avdAudioPlayerConfigDefaults(AVD_AudioPlayerConfig *config);

// this function will block until the audio clip has finished playing
bool avdAudioPlayerPlaySync(AVD_AudioPlayer *player, AVD_AudioClip *clip, AVD_AudioPlayerConfig config);

bool avdAudioPlayerPlayAsync(AVD_AudioPlayer *player, AVD_AudioClip *clip, AVD_Bool ownClip, AVD_AudioPlayerConfig config);
bool avdAudioPlayerStop(AVD_AudioPlayer *player); // stops all playing clips, and frees owned clips

// resoponsible for updating the audio player state, should be called regularly
// this will also free any owned audio clips that have finished playing
bool avdAudioPlayerUpdate(AVD_AudioPlayer *player);

#endif // AVD_AUDIO_PLAYER_H