#pragma once
#include <stddef.h>

#include "types.h"
#include "chip8.h"

void copy_chip_program(const u16* program, u8* location, size_t size);

u16 swap_bytes(u16 num);

void load_and_execute(const u16* program, Chip8* chip, size_t size, size_t num_cycles);