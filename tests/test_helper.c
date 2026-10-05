#include "test_helper.h"

void copy_chip_program(const u16* program, u8* location, const size_t size) {
    const auto dest = (u16*)location;

    for (size_t i = 0; i < size; i++)
        dest[i] = swap_bytes(program[i]);
}

u16 swap_bytes(const u16 num)
{
    const u16 res = ((num & 0xFF00) >> 8) | ((num & 0x00FF) << 8);
    return res;
}

void load_and_execute(const u16* program, Chip8* chip, const size_t size,const size_t num_cycles)
{
    chip8_reset(chip);
    chip->PC = 0;
    copy_chip_program(program, chip->memory, size);
    chip8_cycle_n(chip, num_cycles);
}
