
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
    chip->cpu.PC = 0x200; // Programs are loaded at position 0x200 into memory
    memcpy(chip->memory + 0x050, CHIP8_INITIAL_FONT, 80); // Load font into memory
}