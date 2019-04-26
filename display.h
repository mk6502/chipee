#ifndef CHIPEE_DISPLAY_H_
#define CHIPEE_DISPLAY_H_
#include <SDL2/SDL.h>

void init_chipee_display();
void draw_screen(unsigned char* gfx);
void sdl_event_handler(unsigned char* keypad);
int should_quit();
void stop_chipee_display();

#endif
