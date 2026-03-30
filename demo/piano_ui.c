#include "piano_ui.h"

void draw_piano(SDL_Renderer *renderer, int offsx, int offsy, bool keys[12]) {
    SDL_Rect rect = {0};
    rect.x = offsx;
    rect.y = offsy;
    rect.w = KEY_WIDTH;
    rect.h = KEY_HEIGHT;
    
    for (int i = 0; i < 7; i++) {
        if (keys[i]) SDL_SetRenderDrawColor(renderer, 200, 200, 255, 255);
        else SDL_SetRenderDrawColor(renderer, 244, 244, 244, 255);
        SDL_RenderFillRect(renderer, &rect);
        rect.x += 4 + KEY_WIDTH;
    }

    rect.w /= 2;
    rect.h *= 0.65f;
    rect.x = offsx + 18 + KEY_WIDTH/2;

    for (int i = 0; i < 5; i++) {
        if (keys[i+7]) SDL_SetRenderDrawColor(renderer, 128, 16, 16, 255);
        else SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
        SDL_RenderFillRect(renderer, &rect);
        rect.x += (4 + KEY_WIDTH) * ((i == 1) ? 2 : 1);
    }
}
