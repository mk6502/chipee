#include <stdio.h>
#include <unistd.h>
#include "chipee.h"
#include "display.h"


int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: chipee rom.ch8\n");
        return 1;
    }

    // initialize CPU
    init_cpu();

    // load ROM
    load_rom(argv[1]);

    // initialize display
    init_chipee_display();

    // game loop
    while (1) {
        emulate_cycle();
        sdl_event_handler(keypad);

        if (should_quit()) {
            break;
        }

        if (draw_flag) {
            draw_screen(gfx);
        }

        // hack to limit fps
        usleep(1000);
    }

    stop_chipee_display();
    return 0;
}
