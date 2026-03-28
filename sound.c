#include <math.h>
#include <SDL2/SDL.h>
#include "sound.h"

// all sound-related code is almost verbatim from https://stackoverflow.com/a/45002609

static int sample_nr = 0;

void audio_callback(void *user_data, unsigned char *raw_buffer, int bytes) {
    double sample_rate = 44100.0;
    int amplitude = 28000;

    Sint16* buffer = (Sint16*) raw_buffer;
    int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
    int nr = *(int*) user_data;

    for (int i = 0; i < length; i++, nr++) {
        double time = nr / sample_rate;
        buffer[i] = (Sint16)(amplitude * sin(2.0f * M_PI * 440.0f * time)); // render 440 Hz sine wave
    }
}

void start_sound() {
    SDL_PauseAudio(0);
}

void stop_sound() {
    SDL_PauseAudio(1);
}

void init_chipee_sound() {
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return;
    }

    SDL_AudioSpec want;
    SDL_zero(want);

    want.freq = 44100; // number of samples per second
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512; // buffer-size (small for low-latency short beeps)
    want.callback = audio_callback; // function SDL calls periodically to refill the buffer
    want.userdata = &sample_nr; // counter, keeping track of current sample number

    SDL_AudioSpec have;
    if (SDL_OpenAudio(&want, &have) != 0)
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
    if (want.format != have.format) SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
}

void stop_chipee_sound() {
    SDL_CloseAudio();
}
