#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "chipee.h"
#include "display.h"


int main(int argc, char** argv) {
    // initialize CPU
    init_cpu();

    // load ROM
    // load_rom("roms/ibm.ch8");
    // load_rom("roms/logo.ch8");
    // load_rom("roms/sierpinski.ch8");
    // load_rom("roms/trip8.ch8");
    // load_rom("roms/maze.ch8");
    load_rom("roms/pong1p.ch8");

    // initialize display
    init_chipee_display();

    // game loop
    while (1) {
        emulate_cycle();
        sdl_event_handler(keypad);

        if (should_quit()) {
            stop_chipee_display();
            return 0;
        }

        if (draw_flag) {
            draw_screen(gfx);
        }

        // hack to limit fps
        usleep(1000);
    }

    stop_chipee_display();
    return 1;
}
