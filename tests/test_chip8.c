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

    TEST_ASSERT_EQUAL(chip->cpu.PC, 0x200);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(chip->memory + 0x050, CHIP8_INITIAL_FONT, 80);

    chip8_destroy(chip);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cpu_reset);

    return UNITY_END();
}