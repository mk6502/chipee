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

void init_cpu() {
    srand((unsigned int)time(NULL));

    // load fonts:
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }
}

void load_rom(char* filename) {
    FILE* fileptr = fopen(filename, "rb");
    fseek(fileptr, 0, SEEK_END); // Jump to the end of the file
    long buffer_size = ftell(fileptr); // Get the current byte offset in the file
    rewind(fileptr); // Jump back to the beginning of the file

    char* buffer = (char *)malloc((buffer_size+1)*sizeof(char)); // Enough memory for file + \0
    fread(buffer, buffer_size, 1, fileptr); // Read in the entire file
    fclose(fileptr); // Close the file
    for (int i = 0; i < buffer_size; i++) {
        memory[512 + i] = buffer[i]; // first 512 bytes are reserved
    }
    free(buffer);
}

void emulate_cycle() {
    draw_flag = 0;
    unsigned short op = memory[pc] << 8 | memory[pc + 1];
    // printf("op: %X\n", op);
    // printf("pc: %X\n", pc);
    unsigned short x = 0;
    unsigned short y = 0;

    switch(op & 0xF000) {
        case 0x0000:
            switch (op & 0x000F) {
                case 0x0000: // 0x00E0: clear the screen
                    // printf("clear screen\n");
                    for (int i = 0; i < 64 * 32; i++) {
                        gfx[i] = 0;
                    }
                    pc += 2;
                    break;
                case 0x000E: // 0x00EE: return from subroutine
                    // printf("return from subroutine\n");
                    pc = stack[sp];
                    sp--;
                    pc += 2;
                    break;
                default:
                    printf("Unknown op: 0x%X\n", op);
                    break;
            }
            break;
        case 0x1000:
            // printf("jump to address NNN\n");
            pc = op & 0x0FFF;
            break;
        case 0x2000:
            // printf("call subroutine at NNN\n");
            sp++; // increment the position we are at on the stack
            stack[sp] = pc; // we will need to return eventually, so keep our counter in the stack
            pc = op & 0x0FFF; // set the program counter to NNN
            break;
        case 0x3000:
            // printf("3XNN: skip next instruction if Vx=NN\n");
            x = (op & 0x0F00) >> 8;
            if (V[x] == (op & 0x00FF)) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0x4000:
            // printf("4XNN: skip next instruction if Vx!=NN\n");
            x = (op & 0x0F00) >> 8;
            if (V[x] != (op & 0x00FF)) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0x5000:
            // printf("5XY0: skip next instruction if Vx=Vy\n");
            x = (op & 0x0F00) >> 8;
            y = (op & 0x00F0) >> 4;
            if (V[x] == V[y]) {
                pc += 4;
            }
            else {
                pc += 2;
            }
            break;
        case 0x6000: // 6XNN: Set Vx = NN
            // printf("Set Vx = NN\n");
            x = (op & 0x0F00) >> 8;
            // Why 8?
            // op = 0x6108 ANDed with 0x0F00 = 0x0100 which is decimal 256, binary: 100000000
            // so you need to shift it right 8 places
            V[x] = op & 0x00FF;
            pc += 2;
            break;
        case 0x7000:
            // printf("7XNN: add NN to Vx\n");
            x = (op & 0x0F00) >> 8;
            V[x] += op & 0x00FF;
            pc += 2;
            break;
        case 0x8000:
            x = (op & 0x0F00) >> 8;
            y = (op & 0x00F0) >> 4;

            switch(op & 0x000F) {
                case 0x0000:
                    // printf("8XY0: set Vx = Vy\n");
                    V[x] = V[y];
                    pc += 2;
                    break;
                case 0x0001:
                    // printf("8XY1: set Vx = Vx OR Vy\n");
                    V[x] = V[x] | V[y];
                    pc += 2;
                    break;
                case 0x0002:
                    // printf("8XY2: set Vx = Vx AND Vy\n");
                    V[x] = V[x] & V[y];
                    pc += 2;
                    break;
                case 0x0003:
                    // printf("8XY3: set Vx = Vx XOR Vy\n");
                    V[x] = V[x] ^ V[y];
                    pc += 2;
                    break;
                case 0x0004:
                    // printf("8XY4: set Vx = Vx + Vy, set V[F] = carry\n");
                    V[0xF] = 0;
                    if ((V[x] + V[y]) > 0xFF) {
                        V[0xF] = 1;
                    }
                    V[x] += V[y];
                    pc += 2;
                    break;
                case 0x0005:
                    // printf("8XY5: set Vx = Vx - Vy, set V[F] = NOT borrow\n");
                    // If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
                    V[0xF] = 0;
                    if (V[x] > V[y]) {
                        V[0xF] = 1;
                    }
                    V[x] -= V[y];
                    pc += 2;
                    break;
                // case 0x0006:
                //     // printf("8XY6: set Vx = Vx SHR 1\n");
                //     // If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
                //     pc += 2;
                //     break;
                // case 0x0007:
                //     // printf("8XY7: set Vx = Vy - Vx, set V[F] = NOT borrow\n");
                //     // If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
                //     pc += 2;
                //     break;
                // case 0x000E:
                //     // printf("8XYE: Set Vx = Vx SHL 1\n");
                //     // If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
                //     pc += 2;
                //     break;
                default:
                    printf("Unknown op: 0x%X\n", op);
                    break;
            }
            
            break;
        case 0x9000:
            // printf("9XY0: skip next instruction if Vx != Vy\n");
            x = (op & 0x0F00) >> 8;
            y = (op & 0x00F0) >> 4;
            if (V[x] != V[y]) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0xA000: // ANNN: Set I to the address NNN
            // printf("ANNN: set I to the address NNN\n");
            I = op & 0x0FFF;
            pc += 2;
            break;
        case 0xB000:
            // printf("BNNN: jump to location NNN + V[0]\n");
            pc = (op & 0x0FFF) + V[0];
            break;
        case 0xC000:
            // printf("CXNN: set V[X] = random byte AND NN\n");
            V[(op & 0x0F00)] = rand() & (op & 0x00FF);
            pc += 2;
            break;
        case 0xD000:
            /* 

            Draws a sprite at coordinate (VX, VY)
            that has a width of 8 pixels and a 
            height of N pixels. Each row of 
            8 pixels is read as bit-coded starting
            from memory location I; I value doesn’t
            change after the execution of this 
            instruction. 
            As described above, VF is set to 1 
            if any screen pixels are flipped from set
            to unset when the sprite is
            drawn, and to 0 if that doesn’t happen.

            */
            // printf("0xDXYN: drawing!\n");
            x = V[(op & 0x0F00) >> 8];
            y = V[(op & 0x00F0) >> 4];
            unsigned short height = op & 0x000F;
            unsigned short pixel;

            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        if (gfx[(x + xline + ((y + yline) * 64))] == 1) {
                            V[0xF] = 1;
                        }
                        gfx[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }
 
            draw_flag = 1;
            pc += 2;
            break;
        case 0xE000:
            x = (op & 0x0F00) >> 8;

            switch (op & 0x00FF) {
                case 0x009E:
                    if (keypad[V[x]]) {
                        pc += 2;
                    }
                    pc += 2;
                    break;
                case 0x00A1:
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
            x = (op & 0x0F00) >> 8;
            y = (op & 0x00F0) >> 4;

            switch (op & 0x00FF) {
                case 0x0007:
                    V[x] = delay_timer;
                    pc += 2;
                    break;
                case 0x0015:
                    delay_timer = V[x];
                    pc += 2;
                    break;
                case 0x0018:
                    sound_timer = V[x];
                    pc += 2;
                    break;
                case 0x001E:
                    I += V[x];
                    pc += 2;
                    break;
                case 0x0029:
                    I = V[x] * 0x5;
                    pc += 2;
                    break;
                case 0x0033:
                    memory[I] = V[x] / 100;
                    memory[I + 1] = V[x] % 100 / 10;
                    memory[I + 2] = V[x] % 10;
                    pc += 2;
                    break;
                case 0x0055:
                    for (int i = 0; i <= x; i++) {
                        memory[I + i] = V[i];
                    }
                    pc += 2;
                    break;
                case 0x0065:
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
        printf("BEEP!\n");
        sound_timer--;
    }
}
