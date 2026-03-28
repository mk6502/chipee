#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chipee.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ(desc, actual, expected) do { \
    if ((actual) == (expected)) { \
        tests_passed++; \
    } else { \
        tests_failed++; \
        printf("  FAILED: %s (expected 0x%X, got 0x%X)\n", desc, (unsigned int)(expected), (unsigned int)(actual)); \
    } \
} while(0)

// Helper: reset CPU and inject a single opcode at 0x200
static void setup(unsigned char hi, unsigned char lo) {
    init_cpu();
    memory[0x200] = hi;
    memory[0x201] = lo;
}

// Helper: inject opcode at a specific address
static void inject(unsigned short addr, unsigned char hi, unsigned char lo) {
    memory[addr] = hi;
    memory[addr + 1] = lo;
}

// ==================== Initialization ====================

void test_init() {
    printf("== Initialization ==\n");
    init_cpu();
    ASSERT_EQ("pc starts at 0x200", pc, 0x200);
    ASSERT_EQ("sp starts at 0", sp, 0);
    ASSERT_EQ("I starts at 0", I, 0);
    ASSERT_EQ("V[0] starts at 0", V[0], 0);
    ASSERT_EQ("V[0xF] starts at 0", V[0xF], 0);
    ASSERT_EQ("delay_timer starts at 0", delay_timer, 0);
    ASSERT_EQ("sound_timer starts at 0", sound_timer, 0);
    ASSERT_EQ("draw_flag starts at 0", draw_flag, 0);
    // fonts loaded at 0x050
    ASSERT_EQ("font '0' at 0x050", memory[0x50], 0xF0);
    ASSERT_EQ("font 'F' last byte at 0x09F", memory[0x9F], 0x80);
}

// ==================== 00E0: Clear Screen ====================

void test_00E0() {
    printf("== 00E0: CLS ==\n");
    setup(0x00, 0xE0);
    gfx[0] = 1;
    gfx[100] = 1;
    gfx[64 * 32 - 1] = 1;
    emulate_cycle();
    ASSERT_EQ("gfx[0] cleared", gfx[0], 0);
    ASSERT_EQ("gfx[100] cleared", gfx[100], 0);
    ASSERT_EQ("gfx[last] cleared", gfx[64 * 32 - 1], 0);
    ASSERT_EQ("draw_flag set", draw_flag, 1);
    ASSERT_EQ("pc advanced", pc, 0x202);
}

// ==================== 00EE / 2NNN: Call & Return ====================

void test_2NNN_00EE() {
    printf("== 2NNN/00EE: CALL & RET ==\n");
    setup(0x24, 0x00);  // CALL 0x400
    inject(0x400, 0x00, 0xEE);  // RET at 0x400
    emulate_cycle();  // execute CALL
    ASSERT_EQ("CALL: pc jumps to 0x400", pc, 0x400);
    ASSERT_EQ("CALL: sp is 1", sp, 1);
    ASSERT_EQ("CALL: stack[0] has return addr", stack[0], 0x200);

    emulate_cycle();  // execute RET
    ASSERT_EQ("RET: pc returns to 0x202", pc, 0x202);
    ASSERT_EQ("RET: sp back to 0", sp, 0);
}

void test_nested_calls() {
    printf("== Nested CALL/RET ==\n");
    setup(0x24, 0x00);           // 0x200: CALL 0x400
    inject(0x202, 0x00, 0x00);   // 0x202: NOP (will be skipped)
    inject(0x400, 0x25, 0x00);   // 0x400: CALL 0x500
    inject(0x500, 0x00, 0xEE);   // 0x500: RET
    inject(0x402, 0x00, 0xEE);   // 0x402: RET

    emulate_cycle();  // CALL 0x400
    ASSERT_EQ("nested: sp=1 after first CALL", sp, 1);
    emulate_cycle();  // CALL 0x500
    ASSERT_EQ("nested: sp=2 after second CALL", sp, 2);
    emulate_cycle();  // RET from 0x500
    ASSERT_EQ("nested: pc=0x402 after first RET", pc, 0x402);
    ASSERT_EQ("nested: sp=1 after first RET", sp, 1);
    emulate_cycle();  // RET from 0x400
    ASSERT_EQ("nested: pc=0x202 after second RET", pc, 0x202);
    ASSERT_EQ("nested: sp=0 after second RET", sp, 0);
}

// ==================== 1NNN: Jump ====================

void test_1NNN() {
    printf("== 1NNN: JP ==\n");
    setup(0x13, 0x50);  // JP 0x350
    emulate_cycle();
    ASSERT_EQ("JP: pc = 0x350", pc, 0x350);
}

// ==================== 3XNN: Skip if VX == NN ====================

void test_3XNN() {
    printf("== 3XNN: SE Vx, byte ==\n");
    setup(0x63, 0x42);  // LD V3, 0x42
    emulate_cycle();
    inject(0x202, 0x33, 0x42);  // SE V3, 0x42 (should skip)
    emulate_cycle();
    ASSERT_EQ("SE skip: pc = 0x206", pc, 0x206);

    setup(0x63, 0x42);
    emulate_cycle();
    inject(0x202, 0x33, 0x99);  // SE V3, 0x99 (should not skip)
    emulate_cycle();
    ASSERT_EQ("SE no skip: pc = 0x204", pc, 0x204);
}

// ==================== 4XNN: Skip if VX != NN ====================

void test_4XNN() {
    printf("== 4XNN: SNE Vx, byte ==\n");
    setup(0x63, 0x42);
    emulate_cycle();
    inject(0x202, 0x43, 0x99);  // SNE V3, 0x99 (should skip)
    emulate_cycle();
    ASSERT_EQ("SNE skip: pc = 0x206", pc, 0x206);

    setup(0x63, 0x42);
    emulate_cycle();
    inject(0x202, 0x43, 0x42);  // SNE V3, 0x42 (should not skip)
    emulate_cycle();
    ASSERT_EQ("SNE no skip: pc = 0x204", pc, 0x204);
}

// ==================== 5XY0: Skip if VX == VY ====================

void test_5XY0() {
    printf("== 5XY0: SE Vx, Vy ==\n");
    setup(0x61, 0x0A);  // LD V1, 0x0A
    inject(0x202, 0x62, 0x0A);  // LD V2, 0x0A
    inject(0x204, 0x51, 0x20);  // SE V1, V2 (equal, skip)
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SE Vx==Vy skip: pc = 0x208", pc, 0x208);

    setup(0x61, 0x0A);
    inject(0x202, 0x62, 0x0B);  // LD V2, 0x0B
    inject(0x204, 0x51, 0x20);  // SE V1, V2 (not equal, no skip)
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SE Vx!=Vy no skip: pc = 0x206", pc, 0x206);
}

// ==================== 6XNN: Load immediate ====================

void test_6XNN() {
    printf("== 6XNN: LD Vx, byte ==\n");
    setup(0x6A, 0xFF);  // LD VA, 0xFF
    emulate_cycle();
    ASSERT_EQ("LD VA = 0xFF", V[0xA], 0xFF);
    ASSERT_EQ("pc advanced", pc, 0x202);
}

// ==================== 7XNN: Add immediate ====================

void test_7XNN() {
    printf("== 7XNN: ADD Vx, byte ==\n");
    setup(0x63, 0x10);  // LD V3, 0x10
    inject(0x202, 0x73, 0x05);  // ADD V3, 0x05
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("ADD: V3 = 0x15", V[3], 0x15);

    // test wrap (no carry flag set)
    setup(0x63, 0xFF);
    inject(0x202, 0x73, 0x02);  // ADD V3, 0x02 (wraps)
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("ADD wrap: V3 = 0x01", V[3], 0x01);
    ASSERT_EQ("ADD wrap: VF unchanged", V[0xF], 0);
}

// ==================== 8XY0: LD Vx, Vy ====================

void test_8XY0() {
    printf("== 8XY0: LD Vx, Vy ==\n");
    setup(0x61, 0x42);  // LD V1, 0x42
    inject(0x202, 0x82, 0x10);  // LD V2, V1
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("LD V2 = V1 = 0x42", V[2], 0x42);
}

// ==================== 8XY1: OR ====================

void test_8XY1() {
    printf("== 8XY1: OR Vx, Vy ==\n");
    setup(0x61, 0x0F);
    inject(0x202, 0x62, 0xF0);
    inject(0x204, 0x81, 0x21);  // OR V1, V2
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("OR: V1 = 0xFF", V[1], 0xFF);
    ASSERT_EQ("OR: VF reset to 0", V[0xF], 0);
}

// ==================== 8XY2: AND ====================

void test_8XY2() {
    printf("== 8XY2: AND Vx, Vy ==\n");
    setup(0x61, 0x0F);
    inject(0x202, 0x62, 0xF0);
    inject(0x204, 0x81, 0x22);  // AND V1, V2
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("AND: V1 = 0x00", V[1], 0x00);
    ASSERT_EQ("AND: VF reset to 0", V[0xF], 0);
}

// ==================== 8XY3: XOR ====================

void test_8XY3() {
    printf("== 8XY3: XOR Vx, Vy ==\n");
    setup(0x61, 0xFF);
    inject(0x202, 0x62, 0x0F);
    inject(0x204, 0x81, 0x23);  // XOR V1, V2
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("XOR: V1 = 0xF0", V[1], 0xF0);
    ASSERT_EQ("XOR: VF reset to 0", V[0xF], 0);
}

// ==================== 8XY4: ADD with carry ====================

void test_8XY4() {
    printf("== 8XY4: ADD Vx, Vy (carry) ==\n");
    // no overflow
    setup(0x61, 0x10);
    inject(0x202, 0x62, 0x20);
    inject(0x204, 0x81, 0x24);  // ADD V1, V2
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("ADD no carry: V1 = 0x30", V[1], 0x30);
    ASSERT_EQ("ADD no carry: VF = 0", V[0xF], 0);

    // overflow
    setup(0x61, 0xFF);
    inject(0x202, 0x62, 0x02);
    inject(0x204, 0x81, 0x24);
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("ADD carry: V1 = 0x01", V[1], 0x01);
    ASSERT_EQ("ADD carry: VF = 1", V[0xF], 1);
}

void test_8XY4_vf_as_operand() {
    printf("== 8XY4: ADD VF edge case ==\n");
    // When X=0xF, VF should hold carry after operation
    setup(0x6F, 0xFF);  // LD VF, 0xFF
    inject(0x202, 0x61, 0x02);  // LD V1, 0x02
    inject(0x204, 0x8F, 0x14);  // ADD VF, V1 (0xFF + 0x02 = overflow)
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("ADD VF edge: VF = 1 (carry wins)", V[0xF], 1);
}

// ==================== 8XY5: SUB with borrow ====================

void test_8XY5() {
    printf("== 8XY5: SUB Vx, Vy ==\n");
    // no borrow
    setup(0x61, 0x10);
    inject(0x202, 0x62, 0x05);
    inject(0x204, 0x81, 0x25);  // SUB V1, V2
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SUB no borrow: V1 = 0x0B", V[1], 0x0B);
    ASSERT_EQ("SUB no borrow: VF = 1", V[0xF], 1);

    // borrow
    setup(0x61, 0x05);
    inject(0x202, 0x62, 0x10);
    inject(0x204, 0x81, 0x25);
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SUB borrow: V1 = 0xF5", V[1], 0xF5);
    ASSERT_EQ("SUB borrow: VF = 0", V[0xF], 0);

    // equal (no borrow, VF = 1)
    setup(0x61, 0x42);
    inject(0x202, 0x62, 0x42);
    inject(0x204, 0x81, 0x25);
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SUB equal: V1 = 0", V[1], 0);
    ASSERT_EQ("SUB equal: VF = 1", V[0xF], 1);
}

// ==================== 8XY6: SHR ====================

void test_8XY6() {
    printf("== 8XY6: SHR Vx ==\n");
    // LSB = 1
    setup(0x61, 0x07);  // V1 = 0b00000111
    inject(0x202, 0x81, 0x06);  // SHR V1
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SHR: V1 = 0x03", V[1], 0x03);
    ASSERT_EQ("SHR: VF = 1 (LSB)", V[0xF], 1);

    // LSB = 0
    setup(0x61, 0x04);  // V1 = 0b00000100
    inject(0x202, 0x81, 0x06);
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SHR: V1 = 0x02", V[1], 0x02);
    ASSERT_EQ("SHR: VF = 0 (LSB)", V[0xF], 0);
}

// ==================== 8XY7: SUBN ====================

void test_8XY7() {
    printf("== 8XY7: SUBN Vx, Vy ==\n");
    // Vy > Vx (no borrow)
    setup(0x61, 0x05);
    inject(0x202, 0x62, 0x10);
    inject(0x204, 0x81, 0x27);  // SUBN V1, V2 (V1 = V2 - V1)
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SUBN no borrow: V1 = 0x0B", V[1], 0x0B);
    ASSERT_EQ("SUBN no borrow: VF = 1", V[0xF], 1);

    // Vy < Vx (borrow)
    setup(0x61, 0x10);
    inject(0x202, 0x62, 0x05);
    inject(0x204, 0x81, 0x27);
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SUBN borrow: V1 = 0xF5", V[1], 0xF5);
    ASSERT_EQ("SUBN borrow: VF = 0", V[0xF], 0);

    // equal (no borrow, VF = 1)
    setup(0x61, 0x42);
    inject(0x202, 0x62, 0x42);
    inject(0x204, 0x81, 0x27);
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SUBN equal: V1 = 0", V[1], 0);
    ASSERT_EQ("SUBN equal: VF = 1", V[0xF], 1);
}

// ==================== 8XYE: SHL ====================

void test_8XYE() {
    printf("== 8XYE: SHL Vx ==\n");
    // MSB = 1
    setup(0x61, 0x81);  // V1 = 0b10000001
    inject(0x202, 0x81, 0x0E);  // SHL V1
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SHL: V1 = 0x02", V[1], 0x02);
    ASSERT_EQ("SHL: VF = 1 (MSB)", V[0xF], 1);

    // MSB = 0
    setup(0x61, 0x41);  // V1 = 0b01000001
    inject(0x202, 0x81, 0x0E);
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SHL: V1 = 0x82", V[1], 0x82);
    ASSERT_EQ("SHL: VF = 0 (MSB)", V[0xF], 0);
}

// ==================== 9XY0: Skip if VX != VY ====================

void test_9XY0() {
    printf("== 9XY0: SNE Vx, Vy ==\n");
    setup(0x61, 0x0A);
    inject(0x202, 0x62, 0x0B);
    inject(0x204, 0x91, 0x20);  // SNE V1, V2 (not equal, skip)
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SNE skip: pc = 0x208", pc, 0x208);

    setup(0x61, 0x0A);
    inject(0x202, 0x62, 0x0A);
    inject(0x204, 0x91, 0x20);  // SNE V1, V2 (equal, no skip)
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SNE no skip: pc = 0x206", pc, 0x206);
}

// ==================== ANNN: Set I ====================

void test_ANNN() {
    printf("== ANNN: LD I, addr ==\n");
    setup(0xA3, 0x45);  // LD I, 0x345
    emulate_cycle();
    ASSERT_EQ("LD I = 0x345", I, 0x345);
}

// ==================== BNNN: Jump V0 + NNN ====================

void test_BNNN() {
    printf("== BNNN: JP V0, addr ==\n");
    setup(0x60, 0x10);  // LD V0, 0x10
    inject(0x202, 0xB3, 0x00);  // JP 0x300 + V0
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("JP V0: pc = 0x310", pc, 0x310);
}

// ==================== CXNN: Random ====================

void test_CXNN() {
    printf("== CXNN: RND Vx, byte ==\n");
    setup(0xC1, 0x00);  // RND V1, 0x00 (always 0)
    emulate_cycle();
    ASSERT_EQ("RND mask 0x00: V1 = 0", V[1], 0);

    // mask 0x0F: result should be 0x00-0x0F
    setup(0xC1, 0x0F);
    emulate_cycle();
    ASSERT_EQ("RND mask 0x0F: upper nibble clear", V[1] & 0xF0, 0);
}

// ==================== DXYN: Draw Sprite ====================

void test_DXYN() {
    printf("== DXYN: DRW Vx, Vy, N ==\n");
    // Draw a single row sprite (0xFF = 8 pixels on) at (0,0)
    setup(0x60, 0x00);  // V0 = 0 (x)
    inject(0x202, 0x61, 0x00);  // V1 = 0 (y)
    inject(0x204, 0xA5, 0x00);  // I = 0x500
    inject(0x206, 0xD0, 0x11);  // DRW V0, V1, 1
    memory[0x500] = 0xFF;  // sprite data: all pixels on
    emulate_cycle(); emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("DRW: gfx[0] = 1", gfx[0], 1);
    ASSERT_EQ("DRW: gfx[7] = 1", gfx[7], 1);
    ASSERT_EQ("DRW: gfx[8] = 0", gfx[8], 0);
    ASSERT_EQ("DRW: VF = 0 (no collision)", V[0xF], 0);
    ASSERT_EQ("DRW: draw_flag set", draw_flag, 1);
}

void test_DXYN_collision() {
    printf("== DXYN: Collision detection ==\n");
    setup(0x60, 0x00);
    inject(0x202, 0x61, 0x00);
    inject(0x204, 0xA5, 0x00);
    inject(0x206, 0xD0, 0x11);  // DRW V0, V1, 1
    memory[0x500] = 0x80;  // one pixel
    gfx[0] = 1;  // pre-set pixel for collision
    emulate_cycle(); emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("DRW collision: gfx[0] = 0 (XOR)", gfx[0], 0);
    ASSERT_EQ("DRW collision: VF = 1", V[0xF], 1);
}

void test_DXYN_wrapping() {
    printf("== DXYN: Coordinate wrapping & clipping ==\n");
    // X coordinate wraps mod 64
    setup(0x60, 0x45);  // V0 = 69 (wraps to 69 % 64 = 5)
    inject(0x202, 0x61, 0x00);  // V1 = 0
    inject(0x204, 0xA5, 0x00);
    inject(0x206, 0xD0, 0x11);
    memory[0x500] = 0x80;  // single pixel at leftmost
    emulate_cycle(); emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("DRW X wrap: pixel at x=5", gfx[5], 1);

    // Y coordinate wraps mod 32
    setup(0x60, 0x00);
    inject(0x202, 0x61, 0x23);  // V1 = 35 (wraps to 35 % 32 = 3)
    inject(0x204, 0xA5, 0x00);
    inject(0x206, 0xD0, 0x11);
    memory[0x500] = 0x80;
    emulate_cycle(); emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("DRW Y wrap: pixel at y=3", gfx[3 * 64], 1);

    // Sprite clips at right edge (doesn't wrap around)
    setup(0x60, 0x3D);  // V0 = 61
    inject(0x202, 0x61, 0x00);
    inject(0x204, 0xA5, 0x00);
    inject(0x206, 0xD0, 0x11);
    memory[0x500] = 0xFF;  // 8 pixels, but only 3 fit (61,62,63)
    emulate_cycle(); emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("DRW clip: pixel at x=61", gfx[61], 1);
    ASSERT_EQ("DRW clip: pixel at x=63", gfx[63], 1);
    // pixel should not wrap to x=0
    ASSERT_EQ("DRW clip: x=0 untouched", gfx[0], 0);
}

// ==================== EX9E / EXA1: Key skip ====================

void test_EX9E() {
    printf("== EX9E: SKP Vx ==\n");
    setup(0x61, 0x05);  // V1 = 5
    inject(0x202, 0xE1, 0x9E);  // SKP V1
    keypad[5] = 1;
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SKP pressed: pc = 0x206", pc, 0x206);

    setup(0x61, 0x05);
    inject(0x202, 0xE1, 0x9E);
    keypad[5] = 0;
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SKP not pressed: pc = 0x204", pc, 0x204);
}

void test_EXA1() {
    printf("== EXA1: SKNP Vx ==\n");
    setup(0x61, 0x05);
    inject(0x202, 0xE1, 0xA1);  // SKNP V1
    keypad[5] = 0;
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SKNP not pressed: pc = 0x206", pc, 0x206);

    setup(0x61, 0x05);
    inject(0x202, 0xE1, 0xA1);
    keypad[5] = 1;
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("SKNP pressed: pc = 0x204", pc, 0x204);
}

// ==================== FX07 / FX15: Delay timer ====================

void test_FX07_FX15() {
    printf("== FX07/FX15: Delay timer ==\n");
    setup(0x61, 0x3C);  // LD V1, 60
    inject(0x202, 0xF1, 0x15);  // LD DT, V1
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("set delay_timer = 60", delay_timer, 60);

    inject(0x204, 0xF2, 0x07);  // LD V2, DT
    emulate_cycle();
    ASSERT_EQ("read delay_timer into V2", V[2], 60);
}

// ==================== FX18: Sound timer ====================

void test_FX18() {
    printf("== FX18: Sound timer ==\n");
    setup(0x61, 0x0A);
    inject(0x202, 0xF1, 0x18);  // LD ST, V1
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("set sound_timer = 10", sound_timer, 10);
}

// ==================== FX1E: Add I ====================

void test_FX1E() {
    printf("== FX1E: ADD I, Vx ==\n");
    setup(0xA2, 0x00);  // I = 0x200
    inject(0x202, 0x61, 0x10);  // V1 = 0x10
    inject(0x204, 0xF1, 0x1E);  // ADD I, V1
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("ADD I: I = 0x210", I, 0x210);
}

// ==================== FX29: Font sprite location ====================

void test_FX29() {
    printf("== FX29: LD F, Vx ==\n");
    setup(0x61, 0x00);  // V1 = 0
    inject(0x202, 0xF1, 0x29);  // LD F, V1
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("font 0: I = 0x50", I, 0x50);

    setup(0x61, 0x0F);  // V1 = 0xF
    inject(0x202, 0xF1, 0x29);
    emulate_cycle(); emulate_cycle();
    ASSERT_EQ("font F: I = 0x9B", I, 0x50 + 0xF * 5);
}

// ==================== FX33: BCD ====================

void test_FX33() {
    printf("== FX33: BCD ==\n");
    setup(0x61, 0xFE);  // V1 = 254
    inject(0x202, 0xA5, 0x00);  // I = 0x500
    inject(0x204, 0xF1, 0x33);  // BCD V1
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("BCD 254: hundreds = 2", memory[0x500], 2);
    ASSERT_EQ("BCD 254: tens = 5", memory[0x501], 5);
    ASSERT_EQ("BCD 254: ones = 4", memory[0x502], 4);

    // test 0
    setup(0x61, 0x00);
    inject(0x202, 0xA5, 0x00);
    inject(0x204, 0xF1, 0x33);
    emulate_cycle(); emulate_cycle(); emulate_cycle();
    ASSERT_EQ("BCD 0: hundreds = 0", memory[0x500], 0);
    ASSERT_EQ("BCD 0: tens = 0", memory[0x501], 0);
    ASSERT_EQ("BCD 0: ones = 0", memory[0x502], 0);
}

// ==================== FX55 / FX65: Store/Load registers ====================

void test_FX55() {
    printf("== FX55: LD [I], Vx ==\n");
    init_cpu();
    V[0] = 0xAA; V[1] = 0xBB; V[2] = 0xCC;
    I = 0x500;
    memory[0x200] = 0xF2; memory[0x201] = 0x55;  // LD [I], V2
    emulate_cycle();
    ASSERT_EQ("store V0", memory[0x500], 0xAA);
    ASSERT_EQ("store V1", memory[0x501], 0xBB);
    ASSERT_EQ("store V2", memory[0x502], 0xCC);
    ASSERT_EQ("I unchanged", I, 0x500);
}

void test_FX65() {
    printf("== FX65: LD Vx, [I] ==\n");
    init_cpu();
    I = 0x500;
    memory[0x500] = 0x11; memory[0x501] = 0x22; memory[0x502] = 0x33;
    memory[0x200] = 0xF2; memory[0x201] = 0x65;  // LD V2, [I]
    emulate_cycle();
    ASSERT_EQ("load V0", V[0], 0x11);
    ASSERT_EQ("load V1", V[1], 0x22);
    ASSERT_EQ("load V2", V[2], 0x33);
    ASSERT_EQ("V3 untouched", V[3], 0);
    ASSERT_EQ("I unchanged", I, 0x500);
}

// ==================== FX0A: Wait for key ====================

void test_FX0A() {
    printf("== FX0A: LD Vx, K (key wait) ==\n");
    init_cpu();
    memory[0x200] = 0xF1; memory[0x201] = 0x0A;  // LD V1, K

    // no key pressed: pc should not advance
    emulate_cycle();
    ASSERT_EQ("no key: pc stays at 0x200", pc, 0x200);

    // simulate key 5 press (with prev_keypad transition)
    memset(prev_keypad, 0, sizeof(prev_keypad));
    keypad[5] = 1;
    emulate_cycle();
    ASSERT_EQ("key pressed: pc still 0x200 (waiting for release)", pc, 0x200);
    ASSERT_EQ("waiting_for_key = 5", waiting_for_key, 5);

    // simulate key 5 release
    keypad[5] = 0;
    emulate_cycle();
    ASSERT_EQ("key released: V1 = 5", V[1], 5);
    ASSERT_EQ("key released: pc = 0x202", pc, 0x202);
    ASSERT_EQ("waiting_for_key reset", waiting_for_key, -1);
}

// ==================== Timer update ====================

void test_update_timers() {
    printf("== update_timers ==\n");
    init_cpu();
    delay_timer = 5;
    sound_timer = 3;

    update_timers();
    ASSERT_EQ("delay_timer decremented to 4", delay_timer, 4);
    ASSERT_EQ("sound_timer decremented to 2", sound_timer, 2);
    ASSERT_EQ("sound_flag set", sound_flag, 1);

    // test timers don't go below 0
    delay_timer = 0;
    sound_timer = 0;
    sound_flag = 0;
    update_timers();
    ASSERT_EQ("delay_timer stays 0", delay_timer, 0);
    ASSERT_EQ("sound_timer stays 0", sound_timer, 0);
    ASSERT_EQ("sound_flag not set when timer=0", sound_flag, 0);
}

// ==================== Main ====================

int main(int argc, char **argv) {
    test_init();
    test_00E0();
    test_2NNN_00EE();
    test_nested_calls();
    test_1NNN();
    test_3XNN();
    test_4XNN();
    test_5XY0();
    test_6XNN();
    test_7XNN();
    test_8XY0();
    test_8XY1();
    test_8XY2();
    test_8XY3();
    test_8XY4();
    test_8XY4_vf_as_operand();
    test_8XY5();
    test_8XY6();
    test_8XY7();
    test_8XYE();
    test_9XY0();
    test_ANNN();
    test_BNNN();
    test_CXNN();
    test_DXYN();
    test_DXYN_collision();
    test_DXYN_wrapping();
    test_EX9E();
    test_EXA1();
    test_FX07_FX15();
    test_FX18();
    test_FX1E();
    test_FX29();
    test_FX33();
    test_FX55();
    test_FX65();
    test_FX0A();
    test_update_timers();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
