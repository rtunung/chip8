#include <stdio.h>

#include <SDL3/SDL.h>

#include "chip8.h"

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
    SDL_Init(SDL_INIT_VIDEO);
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
            printf("Shutting down...\n");
            return 1;
        }

    // Setup timers
    double cpu_accumulator = 0.0;
    double timer_accumulator = 0.0;

    auto previous = SDL_GetPerformanceCounter();
    const double frequency = (double)SDL_GetPerformanceFrequency();

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

            // TODO handle all 16 input keys
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

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}