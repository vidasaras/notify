#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

int main(int argc, char* argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() == -1) {
        printf("Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Rect displayBounds;
    int width, height;
    if (SDL_GetDisplayBounds(0, &displayBounds) == 0) {
        width = displayBounds.w;
        height = displayBounds.h;
    }

    // Load font
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16);
    if (!font) {
        printf("Font error: %s\n", TTF_GetError());
        return 1;
    }

    int top = 20;
    int right = 20;

    const char* text = argv[1];
    int text_width, text_height;
    TTF_SizeText(font, text, &text_width, &text_height);

    int win_w = text_width + 40;
    win_w = win_w > 200 ? win_w : 200;
    int win_h = text_height + 30;
    int x = width - win_w - right;  // right margin
    int y = top;                    // top margin

    SDL_Window* window = SDL_CreateWindow("Notification", x, y, win_w, win_h, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Color color = {255, 255, 255}; // white
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_Rect textRect = {10, 10, textSurface->w, textSurface->h}; // position and size

    SDL_FreeSurface(textSurface);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_RenderPresent(renderer);

    SDL_Delay(3000); // wait 3 seconds

    // Cleanup
    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}

