#include <stdio.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "chipee.h"
#include "display.h"
#include "sound.h"

#define CYCLES_PER_FRAME 8
#define FRAME_DURATION_MS (1000 / 60) // ~16.67ms for 60 fps

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
    if (!load_rom(rom_filename)) {
        return 1;
    }

    // initialize display
    init_chipee_display();

    // initialize sound
    init_chipee_sound();

    // game loop
    while (1) {
        Uint32 frame_start = SDL_GetTicks();

        // execute CPU cycles for this frame (~480 cycles/sec at 60 fps)
        for (int i = 0; i < CYCLES_PER_FRAME; i++) {
            emulate_cycle();
        }

        sdl_event_handler(keypad);
        update_prev_keypad();

        if (should_quit()) {
            break;
        }

        // update timers once per frame = 60 Hz
        update_timers();

        // manage sound based on sound_timer
        if (sound_timer > 0) {
            start_sound();
        } else {
            stop_sound();
        }

        if (draw_flag) {
            draw_screen(gfx);
        }

        // sleep remainder of frame to target 60 fps
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DURATION_MS) {
            SDL_Delay(FRAME_DURATION_MS - frame_time);
        }
    }

    stop_chipee_sound();
    stop_chipee_display();
    return 0;
}
