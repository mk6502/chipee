#include <stdio.h>
#include <stdlib.h>
#include "chipee.h"

#define TEST_ROM_LEN 6
unsigned char test_rom[TEST_ROM_LEN] =
{ 
  0x65, 0xF1, 0x62, 0x08, 0xA2, 0x20
};

void load_test_rom() {
    for (int i = 0; i < TEST_ROM_LEN; i++) {
        memory[512 + i] = test_rom[i]; // first 512 bytes are reserved
    }
}

int main(int argc, char** argv) {
    // initialize CPU
    init_cpu();

    // assert pc = 0x200
    printf("TEST: Assert pc = 0x200 at startup...\n");
    if (pc != 0x200) {
        printf("FAILED!\n");
        printf("pc == 0x%X\n", pc);
        return 1;
    } else {
        printf("PASSED!\n");
    }

    load_test_rom();

    // TODO: assert rom loaded correctly

    printf("TEST: Test pc = 0x202 after one instruction...\n");
    emulate_cycle();
    if (pc != 0x202) {
        printf("FAILED!\n");
        printf("pc == 0x%X\n", pc);
        return 1;
    } else {
        printf("PASSED!\n");
    }

    printf("TEST: LD Vx, byte | 0x65F1 | V[5] == 0xF1\n");
    if (V[5] != 0xF1) {
        printf("FAILED!\n");
        printf("V[5] == 0x%x\n", V[5]);
    } else {
        printf("PASSED!\n");
    }

    printf("TEST: Test 0x6XNN: Set V[X] = NN\n");
    emulate_cycle();
    if (V[2] != 0x08) {
        printf("FAILED!\n");
        printf("V[5] == 0x%x\n", V[5]);
    } else {
        printf("PASSED!\n");
    }

    printf("TEST: Test 0ANNN\n");
    emulate_cycle();
    if (I != 0x220) {
        printf("FAILED!\n");
        printf("I == 0x%x\n", I);
    } else {
        printf("PASSED!\n");
    }

    printf("ALL TESTS PASSED!\n");
    return 0;
}
