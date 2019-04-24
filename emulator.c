#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "chipee.h"

void draw_screen() {
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (gfx[x + (y * 64)]) {
                printf("\u2588");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {
    // load fonts
    load_fonts();

    // load ROM
    // load_rom("roms/ibm.ch8");
    // load_rom("roms/logo.ch8");
    // load_rom("roms/sierpinski.ch8");
    // load_rom("roms/trip8.ch8");
    load_rom("roms/maze.ch8");

    // game loop
    while (1) {
        emulate_cycle();

        if (draw_flag) {
            draw_screen();
        }

        // hack to limit to ~60fps
        usleep(15000);
    }

    return 1;
}
