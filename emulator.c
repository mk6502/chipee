#include <stdio.h>
#include <unistd.h>
#include "chipee.h"
#include "display.h"

// all sound-related code is almost verbatim from https://stackoverflow.com/a/45002609
#include <math.h>
#include <SDL2/SDL.h>

const int AMPLITUDE = 10000;
const int SAMPLE_RATE = 44100;

void audio_callback(void *user_data, Uint8 *raw_buffer, int bytes)
{
    Sint16 *buffer = (Sint16*)raw_buffer;
    int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
    int sample_nr = (*(int*)user_data);

    for(int i = 0; i < length; i++, sample_nr++)
    {
        double time = (double)sample_nr / (double)SAMPLE_RATE;
        buffer[i] = (Sint16)(AMPLITUDE * sin(2.0f * M_PI * 441.0f * time)); // render 441 HZ sine wave
    }
}

void beep_sound() {
    int sample_nr = 0;

    SDL_AudioSpec want;
    want.freq = SAMPLE_RATE; // number of samples per second
    want.format = AUDIO_S16SYS; // sample type (here: signed short i.e. 16 bit)
    want.channels = 1; // only one channel
    want.samples = 2048; // buffer-size
    want.callback = audio_callback; // function SDL calls periodically to refill the buffer
    want.userdata = &sample_nr; // counter, keeping track of current sample number

    SDL_AudioSpec have;
    if(SDL_OpenAudio(&want, &have) != 0) SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
    if(want.format != have.format) SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");

    SDL_PauseAudio(0); // start playing sound
    SDL_Delay(2); // wait while sound is playing
    SDL_PauseAudio(1); // stop playing sound
}


int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: chipee rom.ch8\n");
        return 1;
    }

    char* rom_filename = argv[1];

    // initialize CPU
    init_cpu();

    // check if ROM is accessible
    if (!check_rom(rom_filename)) {
        printf("Unable to open ROM!\n");
        return 1;
    }

    // load ROM
    load_rom(rom_filename);

    // initialize display
    init_chipee_display();

    // initialize sound
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
    }

    // game loop
    while (1) {
        emulate_cycle();
        sdl_event_handler(keypad);

        if (should_quit()) {
            break;
        }

        if (sound_flag) {
            beep_sound();
        }

        if (draw_flag) {
            draw_screen(gfx);
        }

        // hack to limit fps
        usleep(1000);
    }

    // stop sound
    SDL_CloseAudio();

    stop_chipee_display();
    return 0;
}
