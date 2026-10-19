#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "chip8.h"
#include "audio.h"

static constexpr int DISPLAY_SCALE_FACTOR = 8;
static constexpr int DISPLAY_WIDTH = CHIP8_DISPLAY_WIDTH * DISPLAY_SCALE_FACTOR;
static constexpr int DISPLAY_HEIGHT = CHIP8_DISPLAY_HEIGHT * DISPLAY_SCALE_FACTOR;

static constexpr double TIMER_HZ = 60.0;
static constexpr double CPU_HZ = 700.0;


static bool load_rom(const char* file_name, Chip8* chip) {
    FILE *file = fopen(file_name, "rb");

    if (file == nullptr) {
        printf("Couldn't open %s\n", file_name);
        return false;
    }

    constexpr size_t max_read = CHIP8_MEMORY_SIZE - CHIP8_PC_INIT;
    size_t bytes_read = 0;
    while (true) {
        const size_t new_bytes = fread((char*)chip->memory + bytes_read + CHIP8_PC_INIT, sizeof(char), max_read - bytes_read, file);
        bytes_read += new_bytes;
        if (new_bytes == 0 || bytes_read >= max_read)
            break;
    }

    fclose(file);
    return true;
}

static void render_chip8_display(const Chip8* chip, SDL_Renderer* renderer) {
    // Set Background to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw the pixels
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < CHIP8_DISPLAY_PIXELS; i++) {
        if (chip->display[i]) {
            const int xpos = (i % CHIP8_DISPLAY_WIDTH) * DISPLAY_SCALE_FACTOR;
            const int ypos = (i / CHIP8_DISPLAY_WIDTH) * DISPLAY_SCALE_FACTOR;
            SDL_FRect pixel_rect = {.x = (float)xpos, .y = (float)ypos, .w = DISPLAY_SCALE_FACTOR, .h = DISPLAY_SCALE_FACTOR};
            SDL_RenderFillRect(renderer, &pixel_rect);
        }
    }
}

int main(const int argc, char** argv)
{

    // Setup SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_Window* window = SDL_CreateWindow("CHIP-8", DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    SDL_GetWindowSurface(window);
    SDL_UpdateWindowSurface(window);

    Chip8_Debug_Enabled = true;

    // Setup Chip-8
    Chip8* chip = chip8_create();
    chip8_reset(chip);

    if (argc >= 2)
        if (!load_rom(argv[1], chip)) {
            SDL_Quit();
            return 1;
        }

    // Setup timers
    double cpu_accumulator = 0.0;
    double timer_accumulator = 0.0;

    auto previous = SDL_GetPerformanceCounter();
    const double frequency = (double)SDL_GetPerformanceFrequency();

    // Setup Audio
    chip8_beep *beep = malloc(sizeof(chip8_beep));
    if (!beep_init(beep)) {
        SDL_Quit();
        return 1;
    }

    // Main Loop
    bool is_running = true;
    SDL_Event event;
    while(is_running)
    {
        // Handle Events
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_EVENT_QUIT)
                is_running = false;
        }

        // Handle Keyboard
        int num_keys = 0;
        const bool* keyboard = SDL_GetKeyboardState(&num_keys);
        constexpr SDL_Scancode keyboard_scancodes[16] = {
            SDL_SCANCODE_0, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5,
            SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_A, SDL_SCANCODE_B,
            SDL_SCANCODE_C, SDL_SCANCODE_D, SDL_SCANCODE_E, SDL_SCANCODE_F
        };
        memset(chip->keyboard, false, sizeof(bool) * 16);
        for (int i = 0; i < 16; i++) {
            if (keyboard_scancodes[i] >= num_keys)
                continue;
            if (keyboard[keyboard_scancodes[i]])
                chip->keyboard[i] = true;
        }

        // Handle elapsed time
        const auto current = SDL_GetPerformanceCounter();
        const double dt = (double)(current-previous) / frequency;
        previous = current;

        cpu_accumulator += dt;
        timer_accumulator += dt;

        // Tick timers
        constexpr double timer_period = 1.0 / TIMER_HZ;
        while (timer_accumulator >= timer_period) {
            chip8_tick_timers(chip);
            beep->playing = chip->sound_timer > 0;
            timer_accumulator -= timer_period;
        }

        // Run cpu cycles
        constexpr double cpu_period = 1.0 / CPU_HZ;
        while (cpu_accumulator >= cpu_period) {
            chip8_cycle(chip);
            cpu_accumulator -= cpu_period;
        }

        // TODO handle audio

        render_chip8_display(chip, renderer);

        SDL_RenderPresent(renderer);

    }

    beep_destroy(beep);
    free(beep);
    chip8_destroy(chip);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}