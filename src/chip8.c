
#include "chip8.h"

#include <limits.h>
#include <stdio.h>
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
    chip->PC = CHIP8_PC_INIT; // Programs are loaded at position 0x200 into memory
    memcpy(chip->memory + 0x050, CHIP8_INITIAL_FONT, 80); // Load font into memory
}

u16 chip8_fetch(const Chip8 *chip) {
    // Opcodes are stored in big endian. We assume our host is little endian.
    const u8 code[2] = {chip->memory[chip->PC + 1], chip->memory[chip->PC]};
    return *(u16*)code;
}

bool chip8_cycle(Chip8* chip) {
    if (chip->PC >= CHIP8_MEMORY_SIZE) {
        printf("Error: PC points out of bounds.\n");
        return false;
    }

    const u16 opcode = chip8_fetch(chip);
    chip->PC += 2;
    return chip8_handle_opcode(chip, opcode);
}

bool chip8_cycle_n(Chip8* chip, size_t cycles)
{
    for (size_t i = 0; i < cycles; i++) {
        const bool result = chip8_cycle(chip);
        if (!result)
            return false;
    }
    return true;
}

bool chip8_handle_opcode(Chip8* chip, const u16 opcode) {

    const u8 first = (opcode & 0xF000) >> 12;

    switch (first) {
        case 0x0: // diverse cases
        {
            switch (opcode) {
                case 0x00E0: // Clear Display
                    memset(chip->display, 0x0, CHIP8_DISPLAY_PIXELS * sizeof(bool));
                    break;
                case 0x00EE: // return
                {
                    if (chip->SP <= 0) {
                        printf("Error: empty stack\n");
                        return false;
                    }
                    chip->SP--;
                    chip->PC = chip->Stack[chip->SP];
                } break;
                default: // 0NNN: machine routine on address NNN
                    printf("Error: not implemented\n");
                    return false;
            }
        } break;

        case 0x1: // 1NNN -> goto NNN
        {
            const u16 address = opcode & 0x0FFF;
            chip->PC = address;
        } break;

        case 0x2: // 2NNN -> call subroutine on address NNN
        {
            if (chip->SP >= CHIP8_STACK_SIZE) {
                printf("Error: stack overflow\n");
                return false;
            }

            const u16 address = opcode & 0x0FFF;
            chip->Stack[chip->SP] = chip->PC;
            chip->SP++;
            chip->PC = address;
        } break;

        case 0x3: // 3XNN -> if (V[X] == NN)
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = opcode & 0x00FF;
            if (chip->V[reg] == val)
                chip->PC += 2;
        } break;

        case 0x4: // 4XNN -> if (V[X] != NN)
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = opcode & 0x00FF;
            if (chip->V[reg] != val)
                chip->PC += 2;
        } break;

        case 0x5: // 5XY0 -> if (V[X] == V[Y])
        {
            const u8 reg1 = (opcode & 0x0F00) >> 8;
            const u8 reg2 = (opcode & 0x00F0) >> 4;
            if (chip->V[reg1] == chip->V[reg2]) {}
                chip->PC += 2;
        } break;

        case 0x6: // 6XNN -> V[X] = NN
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = (opcode & 0x00FF);
            chip->V[reg] = val;
        } break;

        case 0x7: // 7XNN -> V[X] += NN
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = (opcode & 0x00FF);
            chip->V[reg] += val;
        } break;

        case 0x8: // 8XYZ
        {
            const u8 x= (opcode & 0x0F00) >> 8;
            const u8 y = (opcode & 0x00F0) >> 4;
            const u8 operation = opcode & 0x000F;
            switch (operation) {
                case 0x0: // V[X] = V[Y]
                    chip->V[x] = chip->V[y];
                    break;
                case 0x1: // V[X] |= V[Y]
                    chip->V[x] |= chip->V[y];
                    break;
                case 0x2: // V[X] &= V[Y]
                    chip->V[x] &= chip->V[y];
                    break;
                case 0x3: // V[X] ^= V[Y]
                    chip->V[x] ^= chip->V[y];
                    break;
                case 0x4: // V[X] += V[Y]
                    if ((u16)chip->V[x] + (u16)chip->V[y] > 0xFF)
                        chip->V[0xF] = 1;
                    else
                        chip->V[0xF] = 0;
                    chip->V[x] += chip->V[y];
                    break;
                case 0x5: // V[X] -= V[Y]
                    if ((i16)chip->V[x] - (i16)chip->V[y] < 0)
                        chip->V[0xF] = 0;
                    else
                        chip->V[0xF] = 1;
                    chip->V[x] -= chip->V[y];
                    break;
                case 0x6: // V[X] >>= 1
                    const u8 last_bit = chip->V[x] & 0x01;
                    chip->V[x] >>= 1;
                    chip->V[0xF] = last_bit;
                    break;
                case 0x7: // V[X] = V[Y] - V[X]
                    if ((i16)chip->V[y] - (i16)chip->V[x] < 0)
                        chip->V[0xF] = 0;
                    else
                        chip->V[0xF] = 1;
                    chip->V[x] = chip->V[y] - chip->V[x];
                    break;
                case 0xE: // V[x] <<= 1
                    const u8 first_bit = (chip->V[x] & 80) >> 7;
                    chip->V[x] <<= 1;
                    chip->V[0xF] = first_bit;
                    break;
                default:
                    printf("Error: opcode does not exist\n");
                    return false;
            }
        } break;

        default:
            printf("Error: unknown opcode\n");
            return false;
    }

    return true;
}
