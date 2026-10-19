
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

void chip8_print_debug_info(const Chip8* chip, u16 opcode) {
    const u8 first = (opcode & 0xF000) >> 12;
    const u8 X = (opcode & 0x0F00) >> 8;
    const u8 Y = (opcode & 0x00F0) >> 4;
    const u16 NNN = opcode & 0x0FFF;
    const u8 NN = opcode & 0x00FF;
    const u8 N = opcode & 0x000F;

    printf("PC: 0x%X Opcode: 0x%.4X | ", chip->PC - 2, opcode);

    switch (first) {
        case 0x0: // diverse cases
        {
            switch (opcode) {
                case 0x00E0: // Clear Display
                    printf("Clear Display");
                    break;
                case 0x00EE: // return
                    printf("Return -> 0x%.4X", chip->Stack[chip->SP]);
                    break;
                default: // 0NNN: machine routine on address NNN
                    printf("Machine Routine NOT IMPLEMENTED");
            }
        } break;

        case 0x1: // 1NNN -> goto NNN
            printf("Goto -> 0x%.3X", NNN);
            break;

        case 0x2: // 2NNN -> call subroutine on address NNN
            printf("Call subroutine -> 0x%.3X", NNN);
            break;

        case 0x3: // 3XNN -> if (V[X] == NN)
            printf("If V[%.1X] == 0x%.2X | V[%.1X] = 0x%.1X", X, NN, X, chip->V[X]);
            break;

        case 0x4: // 4XNN -> if (V[X] != NN)
            printf("If V[%.1X] != 0x%.2X | V[%.1X] = 0x%.1X", X, NN, X, chip->V[X]);
            break;

        case 0x5: // 5XY0 -> if (V[X] == V[Y])
            printf("If V[%.1X] == V[%.1X] | V[%.1X] = 0x%.1X, V[%.1X] = 0x%.1X", X, Y, X, chip->V[X], Y, chip->V[Y]);
            break;

        case 0x6: // 6XNN -> V[X] = NN
            printf("Set V[%.1X] = 0x%.2X", X, NN);
            break;

        case 0x7: // 7XNN -> V[X] += NN
            printf("Add V[%.1X] += %.2X", X, NN);
            break;

        case 0x8: // 8XYZ
        {
            const u8 operation = opcode & 0x000F;

            switch (operation) {
                case 0x0: // V[X] = V[Y]
                    printf("Set V[%.1X] = V[%.1X]", X, Y);
                    break;
                case 0x1: // V[X] |= V[Y]
                    printf("Or V[%.1X] |= V[%.1X]", X, Y);
                    break;
                case 0x2: // V[X] &= V[Y]
                    printf("And V[%.1X] &= V[%.1X]", X, Y);
                    break;
                case 0x3: // V[X] ^= V[Y]
                    printf("Xor V[%.1X] ^= V[%.1X]", X, Y);
                    break;
                case 0x4: // V[X] += V[Y]
                    printf("Add V[%.1X] += V[%.1X]", X, Y);
                    break;
                case 0x5: // V[X] -= V[Y]
                    printf("Minus V[%.1X] -= V[%.1X]", X, Y);
                    break;
                case 0x6: // V[X] >>= 1
                    printf("Shift Right V[%.1X] >>= 1", X);
                    break;
                case 0x7: // V[X] = V[Y] - V[X]
                    printf("Minus V[%.1X] = V[%.1X] - V[%.1X]", X, Y, X);
                    break;
                case 0xE: // V[x] <<= 1
                    printf("Shift Left V[%.1X] <<= 1", X);
                    break;
                default:
                    printf("Error: opcode does not exist\n");
            }
        } break;

        case 0x9: // 9XY0 -> if (V[X] != V[Y])
            printf("If V[%.1X] != V[%.1X] | V[%.1X] = 0x%.1X, V[%.1X] = 0x%.1X", X, Y, X, chip->V[X], Y, chip->V[Y]);
            break;

        case 0xA: // ANNN -> I = NNN
            printf("Set I = 0x%.3X", NNN);
            break;

        case 0xB: // BNNN -> I = NNN + V[0]
            printf("Add I = 0x%.3X V[0]", NNN);
            break;

        case 0xC: // CXNN -> V[X] = rand() & NN
            printf("Set V[%.1X] = rand() & 0x%.2X", X, NN);
            break;

        case 0xD: // DXYN -> draw_sprite(V[X], V[Y], N)
            printf("Draw Sprite xPos = V[%.1X] yPos = V[%.1X] height = 0x%.1X ", X, Y, N);
            break;

        case 0xE:
        {
            const u8 key = chip->V[X];

            switch (NN) {
                case 0x9E: // EX9E -> if (key() == V[X])
                    printf("If key() == V[%.1X]", X);
                    break;

                case 0xA1: // EXA1 -> if (key() != V[X])
                    printf("If key() != V[%.1X]", X);
                    break;

                default:
                    printf("Invalid Opcode");
            }
        } break;

        case 0xF:
        {
            switch (NN) {
                case 0x07: // V[X] = delay_timer()
                    printf("Set V[%.1X] = delay_timer", X);
                    break;

                case 0x0A: // V[X] = get_key() IMPORTANT! Halts the cpu until key is pressed!
                        printf("Waiting for keyboard V[%.1X] = current_key_press()", X);
                    break;

                case 0x15: // FX15 -> delay_timer(V[X])
                    printf("Set delay_timer = V[%.1X]", X);
                    break;

                case 0x18: // FX18 -> sound_timer(V[X])
                    printf("Set sound_timer = V[%.1X]", X);
                    break;

                case 0x1E: // FX1E -> I += V[X]
                    printf("Add I += V[%.1X]", X);
                    break;

                case 0x29: // FX29 -> I = sprite_addr(V[X])
                    printf("Set I = sprite_addr(V[%.1X])", X);
                    break;

                case 0x33: // FX33 -> MEM[I] = BCD(V[X])
                    printf("Save Mem[I] = BCD(V[%.1X])", X);
                    break;

                case 0x55: // FX55 -> reg_dump(V[X], &I)
                    printf("Dump Registers Mem[I] = V[0] to V[%.1X]", X);
                    break;

                case 0x65: // FX65 -> ref_load(V[X], &I)
                    printf("Load Registers V[0] to V[%.1X] = Mem[I]", X);
                    break;

                default:
                    printf("Invalid Opcode");
            }
        } break;

        default:
            printf("Invalid opcode");
        }

    printf("\n");
}

bool chip8_handle_opcode(Chip8* chip, const u16 opcode) {

    const u8 first = (opcode & 0xF000) >> 12;
    const u8 X = (opcode & 0x0F00) >> 8;
    const u8 Y = (opcode & 0x00F0) >> 4;
    const u16 NNN = opcode & 0x0FFF;
    const u8 NN = opcode & 0x00FF;

    if (Chip8_Debug_Enabled)
        chip8_print_debug_info(chip,opcode);

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
            // If we hard jump to the current jump instruction, we halt the cpu.
            if (NNN == chip->PC - 2) {
                chip->is_halted = true;
                printf("HALT\n");
            }

            chip->PC = NNN;
        } break;

        case 0x2: // 2NNN -> call subroutine on address NNN
        {
            if (chip->SP >= CHIP8_STACK_SIZE) {
                printf("Error: stack overflow\n");
                return false;
            }

            chip->Stack[chip->SP] = chip->PC;
            chip->SP++;
            chip->PC = NNN;
        } break;

        case 0x3: // 3XNN -> if (V[X] == NN)
            if (chip->V[X] == NN)
                chip->PC += 2;
            break;

        case 0x4: // 4XNN -> if (V[X] != NN)
            if (chip->V[X] != NN)
                chip->PC += 2;
            break;

        case 0x5: // 5XY0 -> if (V[X] == V[Y])
            if (chip->V[X] == chip->V[Y])
                chip->PC += 2;
            break;

        case 0x6: // 6XNN -> V[X] = NN
            chip->V[X] = NN;
            break;

        case 0x7: // 7XNN -> V[X] += NN
            chip->V[X] += NN;
            break;

        case 0x8: // 8XYZ
        {
            const u8 operation = opcode & 0x000F;

            switch (operation) {
                case 0x0: // V[X] = V[Y]
                    chip->V[X] = chip->V[Y];
                    break;
                case 0x1: // V[X] |= V[Y]
                    chip->V[X] |= chip->V[Y];
                    break;
                case 0x2: // V[X] &= V[Y]
                    chip->V[X] &= chip->V[Y];
                    break;
                case 0x3: // V[X] ^= V[Y]
                    chip->V[X] ^= chip->V[Y];
                    break;
                case 0x4: // V[X] += V[Y]
                    const int add_total = (int)chip->V[X] + (int)chip->V[Y];
                    chip->V[X] += chip->V[Y];

                    if (add_total > 0xFF)
                        chip->V[0xF] = 1;
                    else
                        chip->V[0xF] = 0;
                    break;
                case 0x5: // V[X] -= V[Y]
                    const u8 result = chip->V[X] - chip->V[Y];

                    if ((i16)chip->V[X] - (i16)chip->V[Y] < 0)
                        chip->V[0xF] = 0;
                    else
                        chip->V[0xF] = 1;

                    if (X != 0xF)
                        chip->V[X] = result;
                    break;
                case 0x6: // V[X] >>= 1
                    const u8 last_bit = chip->V[X] & 0x01;
                    chip->V[X] >>= 1;
                    chip->V[0xF] = last_bit;
                    break;
                case 0x7: // V[X] = V[Y] - V[X]
                    const int sub_total = (int)chip->V[Y] - (int)chip->V[X];
                    chip->V[X] = chip->V[Y] - chip->V[X];

                    if (sub_total < 0)
                        chip->V[0xF] = 0;
                    else
                        chip->V[0xF] = 1;
                    break;
                case 0xE: // V[x] <<= 1
                    const u8 first_bit = (chip->V[X] & 0x80) >> 7;
                    chip->V[X] <<= 1;
                    chip->V[0xF] = first_bit;
                    break;
                default:
                    printf("Error: opcode does not exist\n");
                    return false;
            }
        } break;

        case 0x9: // 9XY0 -> if (V[X] != V[Y])
            if (chip->V[X] != chip->V[Y])
                chip->PC += 2;
            break;

        case 0xA: // ANNN -> I = NNN
            chip->I = NNN;
            break;

        case 0xB: // BNNN -> I = NNN + V[0]
            chip->I = chip->V[0] + NNN;
            break;

        case 0xC: // CXNN -> V[X] = rand() & NN
            chip->V[X] = rand() & NN;
            break;

        case 0xD: // DXYN -> draw_sprite(V[X], V[Y], N)
            const u8 x = chip->V[X];
            const u8 y = chip->V[Y];
            const u8 N = opcode & 0x000F;
            chip8_draw_sprite(chip, x, y, N);;
            break;

        case 0xE:
        {
            const u8 key = chip->V[X];
            if (key > 16)
                break;

            switch (NN) {
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
            switch (NN) {
                case 0x07: // V[X] = delay_timer()
                    chip->V[X] = chip->delay_timer;
                    break;

                case 0x0A: // V[X] = get_key() IMPORTANT! Halts the cpu until key is pressed!

                    if (chip->currently_holding_key && !chip->keyboard[chip->currently_held_key]) {
                            chip->currently_holding_key = false;
                            break;
                    }

                    for (int key = 0; key < CHIP8_KEYBOARD_SIZE; key++)
                        if (chip->keyboard[key]) {
                            chip->V[X] = key;
                            chip->currently_held_key = key;
                            chip->currently_holding_key = true;
                            break;
                        }

                    chip->PC -= 2;
                    break;

                case 0x15: // FX15 -> delay_timer(V[X])
                    chip->delay_timer = chip->V[X];
                    break;

                case 0x18: // FX18 -> sound_timer(V[X])
                    chip->sound_timer = chip->V[X];
                    break;

                case 0x1E: // FX1E -> I += V[X]
                    chip->I += chip->V[X];
                    break;

                case 0x29: // FX29 -> I = sprite_addr(V[X])
                    const u8 char_requested = chip->V[X] & 0x0F;
                    const size_t location = char_requested  * 5 + CHIP8_INITIAL_FONT_LOCATION;
                    chip->I = location;
                    break;

                case 0x33: // FX33 -> MEM[I] = BCD(V[X])
                    u8 value = chip->V[X];
                    for (size_t i = 0; i < 3; i++) {
                        chip->memory[chip->I + 2 - i] = value % 10;
                        value /= 10;
                    }
                    break;

                case 0x55: // FX55 -> reg_dump(V[X], &I)
                    memcpy(chip->memory + chip->I, chip->V, X + 1);
                    break;

                case 0x65: // FX65 -> ref_load(V[X], &I)
                    memcpy(chip->V, chip->memory + chip->I, X + 1);
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

    return true;
}

void chip8_draw_sprite(Chip8* chip, const u8 x, const u8 y, const u8 n) {
    bool pixel_flipped = false;

    for (int row = 0; row < n; row++) {

        const u8 current_row = chip->memory[chip->I + row];
        for (int col = 0; col < 8; col++) {
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
