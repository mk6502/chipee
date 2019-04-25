//
// CHIPEE
//

#ifndef CHIPEE_H_
#define CHIPEE_H_

extern unsigned char chip8_fontset[80];
extern unsigned char memory[4096];
extern unsigned char V[16];
extern unsigned short I;
extern unsigned short pc;
extern unsigned char gfx[64 * 32];
extern unsigned short stack[16];
extern unsigned short sp;
extern unsigned char keypad[16];
extern unsigned char delay_timer;
extern unsigned char sound_timer;
extern unsigned char draw_flag;

void init_cpu();
void load_rom(char* filename);
void emulate_cycle();
int main(int argc, char** argv);

#endif
