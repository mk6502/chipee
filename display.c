#include <SDL2/SDL.h>
#include "display.h"

SDL_Window* screen;
SDL_Renderer* renderer;
// Standard CHIP-8 keypad mapping:
// CHIP-8 key → Keyboard key
// 1 2 3 C      1 2 3 4
// 4 5 6 D      Q W E R
// 7 8 9 E      A S D F
// A 0 B F      Z X C V
SDL_Scancode keymappings[16] = {
        SDL_SCANCODE_X, // 0
        SDL_SCANCODE_1, // 1
        SDL_SCANCODE_2, // 2
        SDL_SCANCODE_3, // 3
        SDL_SCANCODE_Q, // 4
        SDL_SCANCODE_W, // 5
        SDL_SCANCODE_E, // 6
        SDL_SCANCODE_R, // 7
        SDL_SCANCODE_A, // 8
        SDL_SCANCODE_S, // 9
        SDL_SCANCODE_D, // A
        SDL_SCANCODE_F, // B
        SDL_SCANCODE_Z, // C
        SDL_SCANCODE_4, // D
        SDL_SCANCODE_C, // E
        SDL_SCANCODE_V  // F
};
int SHOULD_QUIT = 0;

void init_chipee_display() {
    SDL_Init(SDL_INIT_VIDEO);

    screen = SDL_CreateWindow(
            "Chipee",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            64 * 8,
            32 * 8,
            0
    );
    renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);
}

void draw_screen(unsigned char* gfx) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (gfx[x + (y * 64)]) {
                SDL_Rect rect;
                rect.x = x * 8;
                rect.y = y * 8;
                rect.w = 8;
                rect.h = 8;
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void sdl_event_handler(unsigned char* keypad) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            SHOULD_QUIT = 1;
            return;
        }
    }

    const Uint8* state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_ESCAPE]) {
        SHOULD_QUIT = 1;
    }

    for (int keycode = 0; keycode < 16; keycode++) {
        keypad[keycode] = state[keymappings[keycode]];
    }
}

int should_quit() {
    return SHOULD_QUIT;
}

void stop_chipee_display() {
    SDL_DestroyWindow(screen);
    SDL_Quit();
}
