#ifndef PIANO_UI_H_
#define PIANO_UI_H_

#include <SDL2/SDL.h>
#include <stdbool.h>

#define KEY_WIDTH 70
#define KEY_HEIGHT 300

void draw_piano(SDL_Renderer *renderer, int offsx, int offsy, bool keys[12]);

#endif
