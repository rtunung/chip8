
#include "chip8.h"

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
    chip->cpu.PC = CHIP8_PC_INIT; // Programs are loaded at position 0x200 into memory
    memcpy(chip->memory + 0x050, CHIP8_INITIAL_FONT, 80); // Load font into memory
}

u16 chip8_fetch(const Chip8 *chip) {
    // Opcodes are stored in big endian. We assume our host is little endian.
    const u8 code[2] = {chip->memory[chip->cpu.PC + 1], chip->memory[chip->cpu.PC]};
    return *(u16*)code;
}

bool chip8_cycle(Chip8* chip) {
    if (chip->cpu.PC >= CHIP8_MEMORY_SIZE) {
        printf("Error: PC points out of bounds.\n");
        return false;
    }

    const u16 opcode = chip8_fetch(chip);
    return chip8_handle_opcode(chip, opcode);
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
                    if (chip->cpu.SP <= 0) {
                        printf("Error: empty stack\n");
                        return false;
                    }
                    chip->cpu.SP--;
                    chip->cpu.PC = chip->cpu.Stack[chip->cpu.SP];
                } break;
                default: // 0NNN: machine routine on address NNN
                    printf("Error: not implemented\n");
                    return false;
            }
        } break;

        case 0x1: // 1NNN -> goto NNN
        {
            const u16 address = opcode & 0x0FFF;
            chip->cpu.PC = address;
        } break;

        case 0x2: // 2NNN -> call subroutine on address NNN
        {
            if (chip->cpu.SP >= CHIP8_STACK_SIZE) {
                printf("Error: stack overflow\n");
                return false;
            }

            const u16 address = opcode & 0x0FFF;
            chip->cpu.Stack[chip->cpu.SP] = chip->cpu.PC;
            chip->cpu.SP++;
            chip->cpu.PC = address;
        } break;

        case 0x3: // 3XNN -> if (V[X] == NN)
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = opcode & 0x00FF;
            if (chip->cpu.V[reg] == val)
                chip->cpu.PC += 2;
        }

        case 0x4: // 4XNN -> if (V[X] != NN)
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = opcode & 0x00FF;
            if (chip->cpu.V[reg] != val)
                chip->cpu.PC += 2;
        }

        case 0x5: // 5XY0 -> if (V[X] == V[Y])
        {
            const u8 reg1 = (opcode & 0x0F00) >> 8;
            const u8 reg2 = (opcode & 0x00F0) >> 4;
            if (chip->cpu.V[reg1] == chip->cpu.V[reg2]) {}
                chip->cpu.PC += 2;
        }

        case 0x6: // 6XNN -> V[X] = NN
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = (opcode & 0x00FF);
            chip->cpu.V[reg] = val;
        }

        case 0x7: // 7XNN -> V[X] += NN
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = (opcode & 0x00FF);
            chip->cpu.V[reg] += val;
        }

        default:
            printf("Error: unknown opcode\n");
            return false;
    }

    return true;
}
