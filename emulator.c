#include <stdio.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "chipee.h"
#include "display.h"
#include "sound.h"

#define CYCLES_PER_FRAME 10
#define TIMER_INTERVAL_MS (1000 / 60) // ~16.67ms for 60 Hz

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
    Uint32 last_timer_tick = SDL_GetTicks();

    while (1) {
        // execute multiple CPU cycles per frame
        for (int i = 0; i < CYCLES_PER_FRAME; i++) {
            emulate_cycle();
        }

        sdl_event_handler(keypad);
        update_prev_keypad();

        if (should_quit()) {
            break;
        }

        // update timers at 60 Hz
        Uint32 now = SDL_GetTicks();
        if (now - last_timer_tick >= TIMER_INTERVAL_MS) {
            update_timers();
            last_timer_tick = now;
        }

        // manage sound based on sound_timer
        if (sound_timer > 0) {
            start_sound();
        } else {
            stop_sound();
        }

        if (draw_flag) {
            draw_screen(gfx);
        }

        // sleep to target ~60 fps
        usleep(1000);
    }

    stop_chipee_sound();
    stop_chipee_display();
    return 0;
}
