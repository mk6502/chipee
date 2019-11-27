#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "chipee.h"

//
// Memory map:
// 0x000-0x1FF (0-511)    - Chip 8 interpreter reserved
// 0x050-0x0A0 (80-160)   - Used for the built-in 4x5 pixel font set (0-F)
// 0x200-0xFFF (512-4095) - Program ROM and work RAM
//

unsigned char chip8_fontset[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// memory
unsigned char memory[4096] = {0};

// cpu registers
unsigned char V[16] = {0};

// index register
unsigned short I = 0;

// program counter
unsigned short pc = 0x200;

// contents of the screen (64*32 pixels)
unsigned char gfx[64 * 32] = {0};

// stack and stack pointer
unsigned short stack[16] = {0};
unsigned short sp = 0;

// keypad
unsigned char keypad[16] = {0};

// timers that count down to zero
unsigned char delay_timer = 0;
unsigned char sound_timer = 0;

// whether we need to update the screen in this cycle
unsigned char draw_flag = 0;

// whether we need to play a sound in this cycle
unsigned char sound_flag = 0;

void init_cpu() {
    srand((unsigned int)time(NULL));

    // load fonts:
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }
}

int check_rom(char* filename) {
    FILE* fileptr;

    if ((fileptr = fopen(filename, "rb"))) {
        return 1;
    }
    else {
        return 0;
    }
}

void load_rom(char* filename) {
    FILE* fileptr = fopen(filename, "rb");

    // get size of the file for malloc:
    fseek(fileptr, 0, SEEK_END);
    long buffer_size = ftell(fileptr);
    rewind(fileptr);

    char* buffer = (char *)malloc((buffer_size+1) * sizeof(char)); // enough memory for file + \0
    fread(buffer, buffer_size, 1, fileptr);
    fclose(fileptr);

    for (int i = 0; i < buffer_size; i++) {
        memory[512 + i] = buffer[i]; // first 512 bytes are reserved
    }

    free(buffer);
}

void emulate_cycle() {
    draw_flag = 0;
    sound_flag = 0;
    unsigned short op = memory[pc] << 8 | memory[pc + 1];
    unsigned short x = (op & 0x0F00) >> 8;
    unsigned short y = (op & 0x00F0) >> 4;

    switch(op & 0xF000) {
        case 0x0000:
            switch (op & 0x00FF) {
                case 0x00E0: // 0x00E0: clear the screen
                    for (int i = 0; i < 64 * 32; i++) {
                        gfx[i] = 0;
                    }
                    pc += 2;
                    break;
                case 0x00EE: // 0x00EE: return from subroutine
                    pc = stack[sp];
                    sp--;
                    pc += 2;
                    break;
                default:
                    printf("Unknown op: 0x%X\n", op);
                    break;
            }
            break;
        case 0x1000: // 0x1NNN: jump to address NNN
            pc = op & 0x0FFF;
            break;
        case 0x2000: // 0x2NNN: call subroutine at NNN
            sp++; // increment the position we are at on the stack
            stack[sp] = pc; // we will need to return eventually, so keep our counter in the stack
            pc = op & 0x0FFF;
            break;
        case 0x3000: // 0x3XNN: skip next instruction if V[x] == NN
            if (V[x] == (op & 0x00FF)) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0x4000: // 0x4XNN: skip next instruction if V[x] != NN
            if (V[x] != (op & 0x00FF)) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0x5000: // 0x5XY0: skip next instruction if V[x] == V[y]
            if (V[x] == V[y]) {
                pc += 4;
            }
            else {
                pc += 2;
            }
            break;
        case 0x6000: // 0x6XNN: Set V[x] = NN
            V[x] = op & 0x00FF;
            pc += 2;
            break;
        case 0x7000: // 0x7XNN: add NN to V[x]
            V[x] += op & 0x00FF;
            pc += 2;
            break;
        case 0x8000:
            switch(op & 0x000F) {
                case 0x0000: // 0x8XY0: set V[x] = V[y]
                    V[x] = V[y];
                    pc += 2;
                    break;
                case 0x0001: // 0x8XY1: set V[x] = V[x] OR V[y]
                    V[x] = V[x] | V[y];
                    pc += 2;
                    break;
                case 0x0002: // 0x8XY2: set V[x] = V[x] AND V[y]
                    V[x] = V[x] & V[y];
                    pc += 2;
                    break;
                case 0x0003: // 0x8XY3: set V[x] = V[x] XOR V[y]
                    V[x] = V[x] ^ V[y];
                    pc += 2;
                    break;
                case 0x0004: // 0x8XY4: set V[x] = V[x] + V[y], set V[F] = carry
                    V[0xF] = 0;
                    if ((V[x] + V[y]) > 0xFF) {
                        V[0xF] = 1;
                    }
                    V[x] += V[y];
                    pc += 2;
                    break;
                case 0x0005: // 0x8XY5: set V[x] = V[x] - V[y], set V[F] = NOT borrow
                    // If V[x] > V[y], set V[F] to 1, otherwise 0.
                    // Then set V[x] = V[x] - V[y]
                    V[0xF] = 0;
                    if (V[x] > V[y]) {
                        V[0xF] = 1;
                    }
                    V[x] -= V[y];
                    pc += 2;
                    break;
                case 0x0006: // 0x8XY6: set V[x] = V[x] SHR 1
                    // Set V[F] to the remainder of dividing by 2, then divide V[x] by 2
                    V[0xF] = V[x] % 2;
                    V[x] /= 2;
                    pc += 2;
                    break;
                case 0x0007: // 0x8XY7: set V[x] = V[y] - V[x], set V[F] = NOT borrow
                    // If V[y] > V[x], set V[F] to 1, otherwise 0. Then set V[x] = V[y] - V[x]
                    V[0xF] = 0;
                    if (V[y] > V[x]) {
                        V[0xF] = 1;
                    }
                    V[x] = V[y] - V[x];
                    pc += 2;
                    break;
                case 0x000E: // 0x8XYE: Set V[x] = V[x] SHL 1
                    // If the most-significant bit of V[x] is 1, set V[F] to 1,
                    // otherwise set to 0. Then V[x] is multiplied by 2.
                    V[0xF] = (V[x] >> 7);
                    V[x] *= 2;
                    pc += 2;
                    break;
                default:
                    printf("Unknown op: 0x%X\n", op);
                    break;
            }
            break;
        case 0x9000: // 0x9XY0: skip next instruction if V[x] != V[y]
            if (V[x] != V[y]) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0xA000: // 0xANNN: Set I to the address NNN
            I = op & 0x0FFF;
            pc += 2;
            break;
        case 0xB000: // 0xBNNN: jump to location NNN + V[0]
            pc = (op & 0x0FFF) + V[0];
            break;
        case 0xC000: // 0xCXNN: set V[X] = random byte AND NN
            V[x] = rand() & (op & 0x00FF);
            pc += 2;
            break;
        case 0xD000: // 0xDXYN: drawing!
            /* 
            Draws a sprite at coordinate (V[x], V[y])
            that has a width of 8 pixels and a 
            height of N pixels. Each row of 
            8 pixels is read as bit-coded starting
            from memory location I; I value doesn’t
            change after the execution of this 
            instruction. 
            As described above, V[F] is set to 1 
            if any screen pixels are flipped from set
            to unset when the sprite is
            drawn, and to 0 if that doesn’t happen.
            */
            draw_flag = 1;

            unsigned short height = op & 0x000F;
            unsigned short pixel;

            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        if (gfx[(V[x] + xline + ((V[y] + yline) * 64))] == 1) {
                            V[0xF] = 1;
                        }
                        gfx[V[x] + xline + ((V[y] + yline) * 64)] ^= 1;
                    }
                }
            }
            pc += 2;
            break;
        case 0xE000:
            switch (op & 0x00FF) {
                case 0x009E: // 0xEX9E: Skip next instruction if keypad[V[x]] is pressed
                    if (keypad[V[x]]) {
                        pc += 2;
                    }
                    pc += 2;
                    break;
                case 0x00A1: // 0xEXA1: Skip next instruction if keypad[V[x]] is NOT pressed
                    if (!keypad[V[x]]) {
                        pc += 2;
                    }
                    pc += 2;
                    break;
                default:
                    printf("Unknown op: 0x%X\n", op);
                    break;
            }
            break;
        case 0xF000:
            switch (op & 0x00FF) {
                case 0x0007: // 0xFX07: set V[x] = delay timer
                    V[x] = delay_timer;
                    pc += 2;
                    break;
                case 0x000A: // 0xFX0A: wait for a key press, then store the value of the key in V[x]
                    for (int i = 0; i < 16; i++) {
                        if (keypad[i]) {
                            V[x] = i;
                            pc += 2;
                            break;
                        }
                    }
                    break;
                case 0x0015: // 0xFX15: set delay timer = V[x]
                    delay_timer = V[x];
                    pc += 2;
                    break;
                case 0x0018: // 0xFX18: set sound timer = V[x]
                    sound_timer = V[x];
                    pc += 2;
                    break;
                case 0x001E: // 0xFX1E: set I = I + V[x]
                    I += V[x];
                    pc += 2;
                    break;
                case 0x0029: // 0xFX29: set I = location of sprite for digit V[x]
                    I = V[x] * 5; // each digit is 5 bytes long
                    pc += 2;
                    break;
                case 0x0033: // 0xFX33: store BCD(V[x]) in I, I+1, I+2
                    // The interpreter takes the decimal value of V[x],
                    // and places the hundreds digit in memory at location in I,
                    // the tens digit at location I+1, and the ones digit at 
                    // location I+2.
                    memory[I] = V[x] / 100;
                    memory[I + 1] = V[x] % 100 / 10;
                    memory[I + 2] = V[x] % 10;
                    pc += 2;
                    break;
                case 0x0055: // 0xFX55: store V[0] through V[x] in memory starting at location I
                    for (int i = 0; i <= x; i++) {
                        memory[I + i] = V[i];
                    }
                    pc += 2;
                    break;
                case 0x0065: // 0xFX65: load V[0] through V[x] from memory starting at location I
                    for (int i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }
                    pc += 2;
                    break;
                default:
                    printf("Unknown op: 0x%X\n", op);
                    break;
            }
            break;
        default:
            printf("Unknown op: 0x%X\n", op);
            break;
    }

    // update delay timer
    if (delay_timer > 0) {
        delay_timer--;
    }
 
    // beep and update sound timer
    if (sound_timer > 0) {
        sound_flag = 1;
        sound_timer--;
    }
}
