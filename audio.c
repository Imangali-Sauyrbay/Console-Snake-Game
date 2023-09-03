#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
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

void playBackgroundMusicAsync(const char* filename, int volume, AudioData** audioData) {
    if (audioData == NULL) {
        return;
    }

    *audioData = (AudioData*)malloc(sizeof(AudioData));
    (*audioData)->bgMusic = Mix_LoadMUS(filename);
    (*audioData)->sound = NULL;
    (*audioData)->isMusicPlaying = 0;
    (*audioData)->isMusicPaused = 0;
    (*audioData)->isSoundPlaying = 0;
    (*audioData)->isSoundPaused = 0;
    (*audioData)->isRepeat = 1;

    if ((*audioData)->bgMusic == NULL) {
        printf("Failed to load background music: %s\n", Mix_GetError());
        free(*audioData);
        *audioData = NULL;
        return;
    }

    Mix_VolumeMusic(volume);
    Mix_PlayMusic((*audioData)->bgMusic, -1);
    (*audioData)->isMusicPlaying = 1;
}

void playSoundAsync(const char* filename, AudioData** audioData) {
    if (audioData == NULL) {
        return;
    }

    *audioData = (AudioData*)malloc(sizeof(AudioData));
    (*audioData)->bgMusic = NULL;
    (*audioData)->sound = Mix_LoadWAV(filename);
    (*audioData)->isMusicPlaying = 0;
    (*audioData)->isMusicPaused = 0;
    (*audioData)->isSoundPlaying = 0;
    (*audioData)->isSoundPaused = 0;
    (*audioData)->isRepeat = 0;

    if ((*audioData)->sound == NULL) {
        printf("Failed to load sound: %s\n", Mix_GetError());
        free(*audioData);
        *audioData = NULL;
        return;
    }

    Mix_PlayChannel(-1, (*audioData)->sound, 0);
    (*audioData)->isSoundPlaying = 1;
}


void playSoundAsyncOnce(const char* filename) {
    AudioData* temp;
    playSoundAsync(filename, &temp);
}


void stopAudio(AudioData* audioData) {
    if (audioData) {
        if (audioData->isMusicPlaying) {
            Mix_HaltMusic();
            Mix_FreeMusic(audioData->bgMusic);
        }

        if (audioData->isSoundPlaying) {
            Mix_HaltChannel(-1);
            Mix_FreeChunk(audioData->sound);
        }

        free(audioData);
    }
}


void pauseAudio(AudioData* audioData) {
    if (audioData) {
        if (audioData->isMusicPlaying && !audioData->isMusicPaused) {
            Mix_PauseMusic();
            audioData->isMusicPaused = 1;
        }

        if (audioData->isSoundPlaying && !audioData->isSoundPaused) {
            Mix_Pause(-1);
            audioData->isSoundPaused = 1;
        }
    }
}


void resumeAudio(AudioData* audioData) {
    if (audioData) {
        if (audioData->isMusicPlaying && audioData->isMusicPaused) {
            Mix_ResumeMusic();
            audioData->isMusicPaused = 0;
        }

        if (audioData->isSoundPlaying && audioData->isSoundPaused) {
            Mix_Resume(-1);
            audioData->isSoundPaused = 0;
        }
    }
}

int initAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL Mixer initialization failed: %s\n", Mix_GetError());
        return 1;
    }

    return 0;
}

void cleanupAudio() {
    Mix_CloseAudio();
    SDL_Quit();
}
