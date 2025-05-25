#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINE_LEN 1024
#define LAST_N_LINES 20

char *utf8_to_latin1(const char *utf8) {
  if (!utf8) return NULL;

  // Worst case: every UTF-8 byte maps to one Latin1 byte
  size_t len = 0;
  const unsigned char *src = (const unsigned char *)utf8;
  while (src[len]) len++;

  char *latin1 = malloc(len + 1); // +1 for null terminator
  if (!latin1) return NULL;

  size_t i = 0;
  while (*src) {
    if (*src < 0x80) {
      latin1[i++] = *src++;
    } else if ((src[0] & 0xE0) == 0xC0 && (src[1] & 0xC0) == 0x80) {
      unsigned int codepoint = ((src[0] & 0x1F) << 6) | (src[1] & 0x3F);
      if (codepoint <= 0xFF) {
        latin1[i++] = (char)codepoint;
      } else {
        latin1[i++] = '?';
      }
      src += 2;
    } else {
      latin1[i++] = '?';
      src++;
    }
  }

  latin1[i] = '\0';
  return latin1;
}

typedef void (*callback_t)(void *);

struct delayed_call {
  int delay;
  callback_t func;
  void *arg;
};

void *delayed_thread(void *data) {
  struct delayed_call *call = data;
  sleep(call->delay);
  call->func(call->arg);
  free(call);
  return NULL;
}

void call_after_delay(int seconds, callback_t func, void *arg) {
  pthread_t thread;
  struct delayed_call *call = malloc(sizeof(struct delayed_call));
  call->delay = seconds;
  call->func = func;
  call->arg = arg;
  pthread_create(&thread, NULL, delayed_thread, call);
  pthread_detach(thread);
}

struct move_args {
  SDL_Window *win;
  int x, y;
};

void move_window(void *arg) {
  struct move_args *args = arg;
  SDL_SetWindowPosition(args->win, args->x, args->y);
  free(args);
}

int main(int argc, char* argv[]) {

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0 || TTF_Init() == -1 || Mix_Init(MIX_INIT_MP3) == 0) {
    printf("Init error: %s\n", SDL_GetError());
    return 1;
  }

  // Open the audio device
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    return 1;
  }

  SDL_Rect displayBounds;
  int width, height;
  if (SDL_GetDisplayBounds(0, &displayBounds) == 0) {
    width = displayBounds.w;
    height = displayBounds.h;
  }

  //get the time
  time_t now = time(NULL);                // Get current time

  const char *filename = "./log.txt";

  char exePath[1024];
  ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
  if (len != -1) {
    exePath[len] = '\0';
  } else {
    perror("readlink");
    return 1;
  }
  char* dirPath = dirname(exePath);

  // Load font
  char fontPath[1060];
  snprintf(fontPath, sizeof(fontPath), "%s/font.ttf", dirPath);
  TTF_Font* font = TTF_OpenFont(fontPath, 16);
  if (!font) {
    printf("Font error: %s\n", TTF_GetError());
    return 1;
  }

  int top = 20;
  int right = 20;

  const char* text = argv[1];
  char *latin1_str = utf8_to_latin1(text);
  int text_width, text_height;
  TTF_SizeText(font, text, &text_width, &text_height);

  SDL_Color color = {255, 255, 255}; // white
  SDL_Color color_b = {0, 0, 0}; // black
                                 //SDL_Surface* textSurface = TTF_RenderText_Solid(font, latin1_str, color);
  SDL_Surface* textSurface = TTF_RenderText_Shaded_Wrapped(font, latin1_str, color, color_b, 560);
  int lineHeight = TTF_FontLineSkip(font);  // includes line spacing
  int numLines = textSurface->h / lineHeight;

  int win_w = text_width + 40;
  win_w = win_w < 600 ? win_w : 600;
  int win_h = text_height * numLines + 30;
  int x = width - win_w - right;  // right margin
  int y = top;                    // top margin

  SDL_Window* window = SDL_CreateWindow("Notification", x, y, win_w, win_h, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Failed to open file for appending");
    return 1;
  }
  fseek(file, 0, SEEK_END);
  long pos = ftell(file);
  int newline_count = 0;

  // Go backwards until we find LAST_N_LINES newlines or reach start
  while (pos > 0 && newline_count <= LAST_N_LINES) {
    fseek(file, --pos, SEEK_SET);
    if (fgetc(file) == '\n') {
      newline_count++;
    }
  }

  // If we didn't find enough lines, rewind to start
  if (pos <= 0) {
    rewind(file);
  }

  // Now read and print the last lines
  char line[MAX_LINE_LEN];
  char content_of_line[3][MAX_LINE_LEN];
  int stack_w, stack_h;
  while (fgets(line, sizeof(line), file)) {
    char *token = strtok(line, " ");

    int i = 0;
    while (token != NULL) {
      strcpy(content_of_line[i], token);
      i++;
      token = strtok(NULL, " ");
    }
    int t_then = atoi(content_of_line[0]);
    double seconds_passed = difftime(now, (time_t)t_then);
    if (seconds_passed < 3){
      stack_w += atoi(content_of_line[1]);
      stack_h += atoi(content_of_line[2]);
      struct move_args *args = malloc(sizeof(struct move_args));
      args->win = window;
      args->x = x;
      args->y = y;
      call_after_delay(seconds_passed, move_window, args);
    }
  }
  fclose(file);

  file = fopen(filename, "a");
  if (!file) {
    perror("Failed to open file for appending");
    return 1;
  }

  fprintf(file, "%d %d %d\n",
      now,
      win_w,
      win_h);
  fclose(file);

  SDL_SetWindowPosition( window,
                           x, y+stack_h);

  SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
  SDL_Rect textRect = {10, 10, textSurface->w, textSurface->h}; // position and size

  SDL_FreeSurface(textSurface);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
  SDL_RenderPresent(renderer);

  // Load the sound
  char soundPath[1060];
  snprintf(soundPath, sizeof(soundPath), "%s/aud.mp3", dirPath);

  // Load the sound
  Mix_Chunk* sound = Mix_LoadWAV(soundPath);
  if (!sound) {
    printf("Failed to load sound! SDL_mixer Error: %s\n", Mix_GetError());
    return 1;
  }

  // Play the sound (loop 0 means play once)
  Mix_PlayChannel(-1, sound, 0);

  // Wait for sound to finish (adjust time based on sound length)
  SDL_Delay(3000);

  // Cleanup
  SDL_DestroyTexture(textTexture);
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  Mix_FreeChunk(sound);
  Mix_CloseAudio();
  Mix_Quit();
  TTF_Quit();
  SDL_Quit();

  return 0;
}

