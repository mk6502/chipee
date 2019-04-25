#ifndef CHIPEE_DISPLAY_H_
#define CHIPEE_DISPLAY_H_
#include <SDL2/SDL.h>

void init_chipee_display();
void draw_screen(unsigned char* gfx);
void sdl_keypress(unsigned char* keypad);
void stop_chipee_display();

extern SDL_Window* screen;

#endif
