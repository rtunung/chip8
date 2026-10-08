#pragma once

#include <stddef.h>

#include "types.h"

constexpr u16 CHIP8_STACK_SIZE = 64;
constexpr u16 CHIP8_MEMORY_SIZE = 4096;
constexpr u16 CHIP8_NUM_REGISTERS = 16;
constexpr u16 CHIP8_PC_INIT = 0x200;
constexpr u16 CHIP8_DISPLAY_WIDTH = 64;
constexpr u16 CHIP8_DISPLAY_HEIGHT = 32;
constexpr u16 CHIP8_DISPLAY_PIXELS = CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT;
constexpr u16 CHIP8_KEYBOARD_SIZE = 16;

constexpr u16 CHIP8_INITIAL_FONT_LOCATION = 0x050;
constexpr u8 CHIP8_INITIAL_FONT[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

typedef struct Chip8_Data
{
    // CPU
    u16 PC; // Program Counter
    u16  I; // Index Register
    i16 SP; // Stack Pointer
    u16 Stack[CHIP8_STACK_SIZE]; // The stack
    u8 V[CHIP8_NUM_REGISTERS]; // 16 General Purpose Registers
    u8 delay_timer; // Delay Timer
    u8 sound_timer; // Sound Timer

    // Peripherals
    u8 memory[CHIP8_MEMORY_SIZE];
    bool display[CHIP8_DISPLAY_PIXELS];
    bool keyboard[CHIP8_KEYBOARD_SIZE];
} Chip8;

Chip8* chip8_create();
void chip8_destroy(Chip8* chip);
void chip8_reset(Chip8* chip);

u16 chip8_fetch(const Chip8* chip);
bool chip8_handle_opcode(Chip8* chip, u16 opcode);

void chip8_draw_sprite(Chip8* chip, u8 x, u8 y, u8 n);

bool chip8_cycle(Chip8* chip);
bool chip8_cycle_n(Chip8* chip, size_t n);

void chip8_tick_timers(Chip8* chip);