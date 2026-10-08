
#include "chip8.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool Chip8_Debug_Enabled = false;


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
    memcpy(chip->memory + CHIP8_INITIAL_FONT_LOCATION, CHIP8_INITIAL_FONT, 80); // Load font into memory
}

u16 chip8_fetch(const Chip8 *chip) {
    // Opcodes are stored in big endian. We assume our host is little endian.
    const u8 code[2] = {chip->memory[chip->PC + 1], chip->memory[chip->PC]};
    return *(u16*)code;
}

bool chip8_cycle(Chip8* chip) {
    if (chip->is_halted)
        return true;

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

void chip8_tick_timers(Chip8* chip) {
    if (chip->sound_timer > 0)
        chip->sound_timer--;
    if (chip->delay_timer > 0)
        chip->delay_timer--;
}

bool chip8_handle_opcode(Chip8* chip, const u16 opcode) {

    const u8 first = (opcode & 0xF000) >> 12;

    if (Chip8_Debug_Enabled)
        printf("PC: 0x%X Opcode: 0x%.4X | ", chip->PC - 2, opcode);

    switch (first) {
        case 0x0: // diverse cases
        {
            switch (opcode) {
                case 0x00E0: // Clear Display
                    if (Chip8_Debug_Enabled)
                        printf("Clear Display");

                    memset(chip->display, 0x0, CHIP8_DISPLAY_PIXELS * sizeof(bool));
                    break;
                case 0x00EE: // return
                {
                    if (Chip8_Debug_Enabled)
                        printf("Return -> 0x%.4X", chip->Stack[chip->SP]);

                    if (chip->SP <= 0) {
                        printf("Error: empty stack\n");
                        return false;
                    }
                    chip->SP--;
                    chip->PC = chip->Stack[chip->SP];
                } break;
                default: // 0NNN: machine routine on address NNN
                    if (Chip8_Debug_Enabled)
                        printf("Machine Routine");
                    printf("Error: not implemented\n");
                    return false;
            }
        } break;

        case 0x1: // 1NNN -> goto NNN
        {
            const u16 address = opcode & 0x0FFF;

            if (Chip8_Debug_Enabled)
                printf("Goto -> 0x%.3X", address);

            if (address == chip->PC - 2) {
                chip->is_halted = true;
                if (Chip8_Debug_Enabled)
                    printf("\nHALT");
            }

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

            if (Chip8_Debug_Enabled)
                printf("Call subroutine -> 0x%.3X", address);
        } break;

        case 0x3: // 3XNN -> if (V[X] == NN)
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = opcode & 0x00FF;
            if (chip->V[reg] == val)
                chip->PC += 2;

            if (Chip8_Debug_Enabled)
                printf("If V[%.1X] == 0x%.2X | V[%.1X] = 0x%.1X", reg, val, reg, chip->V[reg]);
        } break;

        case 0x4: // 4XNN -> if (V[X] != NN)
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = opcode & 0x00FF;
            if (chip->V[reg] != val)
                chip->PC += 2;

            if (Chip8_Debug_Enabled)
                printf("If V[%.1X] != 0x%.2X | V[%.1X] = 0x%.1X", reg, val, reg, chip->V[reg]);
        } break;

        case 0x5: // 5XY0 -> if (V[X] == V[Y])
        {
            const u8 reg1 = (opcode & 0x0F00) >> 8;
            const u8 reg2 = (opcode & 0x00F0) >> 4;
            if (chip->V[reg1] == chip->V[reg2]) {}
                chip->PC += 2;

            if (Chip8_Debug_Enabled)
                printf("If V[%.1X] == V[%.1X] | V[%.1X] = 0x%.1X, V[%.1X] = 0x%.1X", reg1, reg2, reg1, chip->V[reg1], reg2, chip->V[reg2]);
        } break;

        case 0x6: // 6XNN -> V[X] = NN
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = (opcode & 0x00FF);
            chip->V[reg] = val;

            if (Chip8_Debug_Enabled)
                printf("Set V[%.1X] = 0x%.2X", reg, val);
        } break;

        case 0x7: // 7XNN -> V[X] += NN
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 val = (opcode & 0x00FF);
            chip->V[reg] += val;

            if (Chip8_Debug_Enabled)
                printf("Add V[%.1X] += %.2X", reg, val);
        } break;

        case 0x8: // 8XYZ
        {
            const u8 x= (opcode & 0x0F00) >> 8;
            const u8 y = (opcode & 0x00F0) >> 4;
            const u8 operation = opcode & 0x000F;

            switch (operation) {
                case 0x0: // V[X] = V[Y]
                    chip->V[x] = chip->V[y];
                    if (Chip8_Debug_Enabled)
                        printf("Set V[%.1X] = V[%.1X]", x, y);
                    break;
                case 0x1: // V[X] |= V[Y]
                    chip->V[x] |= chip->V[y];
                    if (Chip8_Debug_Enabled)
                        printf("Or V[%.1X] |= V[%.1X]", x, y);
                    break;
                case 0x2: // V[X] &= V[Y]
                    chip->V[x] &= chip->V[y];
                    if (Chip8_Debug_Enabled)
                        printf("And V[%.1X] &= V[%.1X]", x, y);
                    break;
                case 0x3: // V[X] ^= V[Y]
                    chip->V[x] ^= chip->V[y];
                    if (Chip8_Debug_Enabled)
                        printf("Xor V[%.1X] ^= V[%.1X]", x, y);
                    break;
                case 0x4: // V[X] += V[Y]
                    if ((u16)chip->V[x] + (u16)chip->V[y] > 0xFF)
                        chip->V[0xF] = 1;
                    else
                        chip->V[0xF] = 0;
                    chip->V[x] += chip->V[y];
                    if (Chip8_Debug_Enabled)
                        printf("Add V[%.1X] += V[%.1X]", x, y);
                    break;
                case 0x5: // V[X] -= V[Y]
                    if ((i16)chip->V[x] - (i16)chip->V[y] < 0)
                        chip->V[0xF] = 0;
                    else
                        chip->V[0xF] = 1;
                    chip->V[x] -= chip->V[y];
                    if (Chip8_Debug_Enabled)
                        printf("Minus V[%.1X] -= V[%.1X]", x, y);
                    break;
                case 0x6: // V[X] >>= 1
                    const u8 last_bit = chip->V[x] & 0x01;
                    chip->V[x] >>= 1;
                    chip->V[0xF] = last_bit;
                    if (Chip8_Debug_Enabled)
                        printf("Shift Right V[%.1X] >>= 1", x);
                    break;
                case 0x7: // V[X] = V[Y] - V[X]
                    if ((i16)chip->V[y] - (i16)chip->V[x] < 0)
                        chip->V[0xF] = 0;
                    else
                        chip->V[0xF] = 1;
                    chip->V[x] = chip->V[y] - chip->V[x];
                    if (Chip8_Debug_Enabled)
                        printf("Minus V[%.1X] = V[%.1X] - V[%.1X]", x, y, x);
                    break;
                case 0xE: // V[x] <<= 1
                    const u8 first_bit = (chip->V[x] & 80) >> 7;
                    chip->V[x] <<= 1;
                    chip->V[0xF] = first_bit;
                    if (Chip8_Debug_Enabled)
                        printf("Shift Left V[%.1X] <<= 1", x);
                    break;
                default:
                    printf("Error: opcode does not exist\n");
                    return false;
            }
        } break;

        case 0x9: // 9XY0 -> if (V[X] != V[Y])
        {
            const u8 reg1 = (opcode & 0x0F00) >> 8;
            const u8 reg2 = (opcode & 0x00F0) >> 4;
            if (chip->V[reg1] != chip->V[reg2]) {}
            chip->PC += 2;

            if (Chip8_Debug_Enabled)
                printf("If V[%.1X] != V[%.1X] | V[%.1X] = 0x%.1X, V[%.1X] = 0x%.1X", reg1, reg2, reg1, chip->V[reg1], reg2, chip->V[reg2]);
        } break;

        case 0xA: // ANNN -> I = NNN
        {
            const u16 nnn = opcode & 0x0FFF;
            chip->I = nnn;

            if (Chip8_Debug_Enabled)
                printf("Set I = 0x%.3X", nnn);
        } break;

        case 0xB: // BNNN -> I = NNN + V[0]
        {
            const u16 NNN = opcode & 0x0FFF;
            chip->I = chip->V[0] + NNN;

            if (Chip8_Debug_Enabled)
                printf("Add I = 0x%.3X V[0]", NNN);
        } break;

        case 0xC: // CXNN -> V[X] = rand() & NN
        {
            const u8 random = rand();
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 NN = opcode & 0x00FF;
            chip->V[reg] = random & NN;

            if (Chip8_Debug_Enabled)
                printf("Set V[%.1X] = rand() & 0x%.2X", reg, NN);
        } break;

        case 0xD: // DXYN -> draw_sprite(V[X], V[Y], N)
        {
            const u8 x_reg = (opcode & 0x0F00) >> 8;
            const u8 y_reg = (opcode & 0x00F0) >> 4;
            const u8 x = chip->V[x_reg];
            const u8 y = chip->V[y_reg];
            const u8 n = opcode & 0x000F;
            chip8_draw_sprite(chip, x, y, n);;
            if (Chip8_Debug_Enabled)
                printf("Draw Sprite xPos = V[%.1X] yPos = V[%.1X] height = 0x%.1X ", x_reg, y_reg, n);
        } break;

        case 0xE:
        {
            const u8 last_byte = opcode & 0x00FF;
            const u8 reg = (opcode & 0x0F00) >> 8;

            const u8 key = chip->V[reg];
            if (key > 16)
                break;

            switch (last_byte) {
                case 0x9E: // EX9E -> if (key() == V[X])
                    if (chip->keyboard[key])
                        chip->PC += 2;
                    break;

                case 0xA1: // EXA1 -> if (key() != V[X])
                    if (!chip->keyboard[key])
                        chip->PC += 2;
                    break;

                default:
                    printf("Error: invalid opcode\n");
                    return false;
            }
        } break;

        case 0xF:
        {
            const u8 reg = (opcode & 0x0F00) >> 8;
            const u8 last_byte = opcode & 0x00FF;

            switch (last_byte) {
                case 0x15: // FX15 -> delay_timer(V[X])
                    chip->delay_timer = chip->V[reg];
                    break;

                case 0x18: // FX18 -> sound_timer(V[X])
                    chip->sound_timer = chip->V[reg];
                    break;

                case 0x1E: // FX1E -> I += V[X]
                    chip->I += chip->V[reg];
                    break;

                case 0x29: // FX29 -> I = sprite_addr(V[X])
                    const u8 char_requested = chip->V[reg] & 0x0F;
                    const size_t location = char_requested  * 5 + CHIP8_INITIAL_FONT_LOCATION;
                    chip->I = location;
                    break;

                case 0x33: // FX33 -> MEM[I] = BCD(V[X])
                    u8 value = chip->V[reg];
                    for (size_t i = 0; i < 3; i++)
                    {
                        chip->memory[chip->I + 2 - i] = value % 10;
                        value /= 10;
                    }
                    break;

                case 0x55: // FX55 -> reg_dump(V[X], &I)
                    memcpy(chip->memory + chip->I, chip->V, reg + 1);
                    break;

                case 0x65: // FX65 -> ref_load(V[X], &I)
                    memcpy(chip->V, chip->memory + chip->I, reg + 1);
                    break;

                default:
                    printf("Error: invalid opcode\n");
                    return false;
            }
        } break;

        default:
            printf("Error: unknown opcode\n");
            return false;
    }

    if (Chip8_Debug_Enabled)
        printf("\n");

    return true;
}

void chip8_draw_sprite(Chip8* chip, const u8 x, const u8 y, const u8 n)
{
    bool pixel_flipped = false;

    for (int row = 0; row < n; row++)
    {
        const u8 current_row = chip->memory[chip->I + row];
        for (int col = 0; col < 8; col++)
        {
            const bool current_pixel = (current_row >> (7 - col)) & 0x1;

            const size_t x_pos = (x + col) % CHIP8_DISPLAY_WIDTH;
            const size_t y_pos = (y + row) % CHIP8_DISPLAY_HEIGHT;
            const size_t position = x_pos + y_pos * CHIP8_DISPLAY_WIDTH;

            const bool previous_pixel = chip->display[position];
            chip->display[position] ^= current_pixel;

            if (previous_pixel && current_pixel == false)
                pixel_flipped = true;
        }
    }

    chip->V[0xF] = pixel_flipped;
}
