#include <stdio.h>

#include <SDL3/SDL.h>

int main()
{

    SDL_Init(SDL_INIT_VIDEO);

    SDL_LogInfo(0, "Starting...");

    SDL_Window* window = SDL_CreateWindow("Hello World", 800, 300, 0);

    SDL_GetWindowSurface(window);
    SDL_UpdateWindowSurface(window);

    bool is_running = true;
    SDL_Event event;
    while(is_running)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_EVENT_QUIT)
                is_running = false;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}