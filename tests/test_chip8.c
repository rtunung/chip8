#include <string.h>

#include "unity.h"
#include "chip8.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_cpu_reset(void)
{
    Chip8 *chip = chip8_create();
    chip8_reset(chip);

    TEST_ASSERT_EQUAL(chip->PC, CHIP8_PC_INIT);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(chip->memory + 0x050, CHIP8_INITIAL_FONT, 80);

    chip8_destroy(chip);
}

static void test_fetch_endianness(void)
{
    Chip8 *chip = chip8_create();

    constexpr u16 test_number = 0xDD11;
    constexpr u16 test_expected = 0x11DD;
    memcpy(chip->memory + CHIP8_PC_INIT, &test_number, sizeof(u16));
    const u16 test_result = chip8_fetch(chip);
    TEST_ASSERT_EQUAL(test_expected, test_result);

    chip8_destroy(chip);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cpu_reset);
    RUN_TEST(test_fetch_endianness);

    return UNITY_END();
}