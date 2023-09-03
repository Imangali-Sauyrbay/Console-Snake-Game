#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL_mixer.h>

typedef struct {
    Mix_Music* bgMusic;
    Mix_Chunk* sound;
    int isMusicPlaying;
    int isMusicPaused;
    int isSoundPlaying;
    int isSoundPaused;
    int isRepeat;
} AudioData;

void playBackgroundMusicAsync(const char* filename, int volume, AudioData** audioData);
void playAudioAsync(const char* filename, AudioData** audioData);
void playSoundAsyncOnce(const char* filename);
void stopAudio(AudioData* audioData);
void pauseAudio(AudioData* audioData);
void resumeAudio(AudioData* audioData);
void cleanupAudio();
int initAudio();


#endif