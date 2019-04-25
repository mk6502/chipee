#include "display.h"


SDL_Window* screen;
SDL_Renderer* renderer;
SDL_Texture* screen_texture;
SDL_Scancode keymappings[16] = {
    SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
    SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
    SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V
};


void init_chipee_display() {
    SDL_Init(SDL_INIT_EVERYTHING);

    screen = SDL_CreateWindow(
        "Chipee",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        64*8,
        32*8,
        0
    );
    renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);
    screen_texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        64*8,
        32*8
    );
}

void draw_screen(unsigned char* gfx) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (gfx[x + (y * 64)]) {
                SDL_Rect rect;
                rect.x = x*8;
                rect.y = y*8;
                rect.w = 8;
                rect.h = 8;
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void sdl_keypress(unsigned char* keypad) {
    SDL_PumpEvents();

    const Uint8* state = SDL_GetKeyboardState(NULL);

    if (state[SDL_SCANCODE_ESCAPE]) {
        stop_chipee_display();
        // TODO: actually exit the program
    }

    for (int keycode = 0; keycode < 16; keycode++) {
        keypad[keycode] = state[keymappings[keycode]];
    }

    return;
}

void stop_chipee_display() {
    SDL_DestroyWindow(screen);
    SDL_Quit();
}
