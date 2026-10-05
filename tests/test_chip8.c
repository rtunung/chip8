#include <stdlib.h>
#include <string.h>

#include "unity.h"
#include "chip8.h"

#include "test_helper.h"

void setUp()
{
}

void tearDown()
{
}

static void test_cpu_reset()
{
    Chip8 *chip = chip8_create();
    chip8_reset(chip);

    TEST_ASSERT_EQUAL(chip->PC, CHIP8_PC_INIT);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(chip->memory + 0x050, CHIP8_INITIAL_FONT, 80);

    chip8_destroy(chip);
}

static void test_fetch_endianness()
{
    Chip8 *chip = chip8_create();

    constexpr u16 test_number = 0xDD11;
    constexpr u16 test_expected = 0x11DD;
    memcpy(chip->memory + CHIP8_PC_INIT, &test_number, sizeof(u16));
    const u16 test_result = chip8_fetch(chip);
    TEST_ASSERT_EQUAL(test_expected, test_result);

    chip8_destroy(chip);
}

static void test_math_binary_operations() {
    Chip8 *chip = chip8_create();

    for (int i = 0; i < 1000; i++)
    {
        const u8 a = rand();
        const u8 b = rand();
        const u16 load_a1 = 0x6100 | a;
        const u16 load_b2 = 0x6200 | b;

        // Test addition
        u16 test_program[] = {load_a1, load_b2, 0x8124};
        constexpr size_t test_program_size = sizeof(test_program) / sizeof(u16);
        load_and_execute(test_program, chip, test_program_size, 3);

        const u8 expected_overflow = (i16) a + (i16) b > UCHAR_MAX ? 1 : 0;
        TEST_ASSERT_EQUAL(chip->V[1], (u8)(a + b));
        TEST_ASSERT_EQUAL(chip->V[0xF], expected_overflow);

        // Test substraction
        test_program[2] = 0x8125;
        load_and_execute(test_program, chip, test_program_size, 3);

        const u8 expected_underflow = (i16) a - (i16) b < 0 ? 0 : 1;
        TEST_ASSERT_EQUAL(chip->V[1], (u8)(a - b));
        TEST_ASSERT_EQUAL(chip->V[0xF], expected_underflow);

        // Test substraction revered
        test_program[2] = 0x8127;
        load_and_execute(test_program, chip, test_program_size, 3);

        const u8 expected_overflow_rev = (i16) b - (i16) a < 0 ? 0 : 1;
        TEST_ASSERT_EQUAL(chip->V[1], (u8)(b - a));
        TEST_ASSERT_EQUAL(chip->V[0xF], expected_overflow_rev);

        // Test Or
        test_program[2] = 0x8121;
        load_and_execute(test_program, chip, test_program_size, 3);

        TEST_ASSERT_EQUAL(chip->V[1], a | b);

        // Test And
        test_program[2] = 0x8122;
        load_and_execute(test_program, chip, test_program_size, 3);

        TEST_ASSERT_EQUAL(chip->V[1], a & b);

        // Test Xor
        test_program[2] = 0x8123;
        load_and_execute(test_program, chip, test_program_size, 3);

        TEST_ASSERT_EQUAL(chip->V[1], a ^ b);

        // Test Copy
        test_program[2] = 0x8120;
        load_and_execute(test_program, chip, test_program_size, 3);

        TEST_ASSERT_EQUAL(chip->V[1], b);

        // Test Shift Right
        test_program[2] = 0x8126;
        load_and_execute(test_program, chip, test_program_size, 3);

        TEST_ASSERT_EQUAL(chip->V[1], a >> 1);

        // Test Shift Left
        test_program[2] = 0x812E;
        load_and_execute(test_program, chip, test_program_size, 3);

        TEST_ASSERT_EQUAL(chip->V[1], (u8)(a << 1));
    }

    chip8_destroy(chip);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cpu_reset);
    RUN_TEST(test_fetch_endianness);
    RUN_TEST(test_math_binary_operations);

    return UNITY_END();
}