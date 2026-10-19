#pragma once

#include <SDL3/SDL.h>

typedef struct {
    SDL_AudioStream *stream;
    bool playing;
    float phase;
} chip8_beep;

bool beep_init(chip8_beep *beep);
void beep_destroy(chip8_beep *beep);