
#include "chip8.h"

#include <stdlib.h>
#include <string.h>

Chip8* chip8_create()
{
    Chip8* chip = malloc(sizeof(Chip8));
    chip8_reset(chip);
    return chip;
}

void chip8_destroy(Chip8* chip)
{
    free(chip);
}

void chip8_reset(Chip8* chip) {
    memset(chip, 0, sizeof(Chip8));
    chip->cpu.PC = CHIP8_PC_INIT; // Programs are loaded at position 0x200 into memory
    memcpy(chip->memory + 0x050, CHIP8_INITIAL_FONT, 80); // Load font into memory
}

u16 chip8_fetch(const Chip8 *chip) {
    // Opcodes are stored in big endian. We assume our host is little endian.
    const u8 code[2] = {chip->memory[chip->cpu.PC + 1], chip->memory[chip->cpu.PC]};
    return *(u16*)code;
}
