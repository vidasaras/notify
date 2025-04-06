#include <SDL2/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Define your desired distances
    int left = 100;     // distance from left of the screen
    int top = 100;      // distance from top of the screen
    int width = 640;
    int height = 480;

    SDL_Window* window = SDL_CreateWindow(
        "My SDL Window",
        left,        // x position
        top,         // y position
        width,       // width
        height,      // height
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Delay(3000); // Show window for 3 seconds

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

