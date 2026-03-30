#include <stdio.h>
#include <stdbool.h>

#include "soundchip.h"
#include "piano_ui.h"

#include <SDL2/SDL.h>

#define SAMPLE_RATE 44100
#define SAMPLE_COUNT 512
#define WINDOW_SIZE 640

void audio_callback(void *userdata, Uint8 *stream, int len) {
    SoundChip *chip = (SoundChip*)userdata;
    soundchip_generate(chip, stream, len);
}

int find_voice(bool voice_activity[4]) {
    for (int i = 0; i < 4; i++) {
        if (!voice_activity[i])  return i;
    }
    return -1;
}

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Failed to initialize SDL : %s\n", SDL_GetError());
        return 1;
    }

    bool keys[12] = {0};
    bool prev_keys[12] = {0};

    int base_note = 60;

    int voice_note[4];
    bool voice_active[4];
    int key_active_midi[12];

    for (int i = 0; i < 4; i++) {
        voice_note[i] = -1;
        voice_active[i] = false;
    }

    int key_to_midi[12] = {
        0,2,4,5,7,9,11,
        1,3,6,8,10
    };


    WaveType current_wave = WAVE_SINE;
    const char *wave_names[] = {"Sine", "Square", "Triangle", "Noise"};

    SoundChip chip = new_SoundChip(SAMPLE_RATE);
    for (int i = 0; i < 4; i++) {
        chip.voices[i].wave = current_wave;
        chip.voices[i].enabled = true;
        chip.voices[i].env.release = 0.5f;
    }

    SDL_AudioSpec spec = {0};
    spec.freq = SAMPLE_RATE;
    spec.channels = 1;
    spec.samples = SAMPLE_COUNT;
    spec.callback = audio_callback;
    spec.format = AUDIO_F32SYS;
    spec.userdata = &chip;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (dev == 0) {
        fprintf(stderr, "Failed to open audio device : %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Piano", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Failed to create window : %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Failed to create renderer : %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
 
    SDL_PauseAudioDevice(dev, 0);

    printf("Piano controls : A-J (White Keys), W-U (Black Keys)\n");
    printf("Z/X: Shift Octave | Space: Cycle Waveform\n");
    printf("C/V: Master Volume\n");
    printf("--------------------------------------------------\n");
    printf("Current Wave: %s | Base Note: %d | Volume : %d%%\n", wave_names[current_wave], base_note, (int)(chip.volume*100));

    int running = 1;
    while (running) {
        SDL_Event evnt;
        while (SDL_PollEvent(&evnt)) {
            if (evnt.type == SDL_QUIT) {
                running = 0;
            }

            if (evnt.type == SDL_KEYDOWN || evnt.type == SDL_KEYUP) {
                bool pressed = evnt.type == SDL_KEYDOWN;

                if (pressed && !evnt.key.repeat) {
                    switch (evnt.key.keysym.sym) {
                        case SDLK_z:
                            if (base_note >= 24) {
                                base_note -= 12;
                                printf("Octave Down | Base Note : %d\n", base_note);
                            }
                            break;
                        case SDLK_x:
                            if (base_note <= 96) {
                                base_note += 12;
                                printf("Octave Up | Base Note : %d\n", base_note);
                            }
                            break;
                        case SDLK_SPACE:
                            current_wave = (current_wave + 1) % 4;
                            for (int i = 0; i < 4; i++)
                                chip.voices[i].wave = current_wave;
                            printf("Current waveform : %s\n", wave_names[current_wave]);
                            break;
                        case SDLK_c:
                            chip.volume -= 0.1f;
                            if (chip.volume < 0.0f) chip.volume = 0.0f;
                            printf("Volume: %d%%\n", (int)(chip.volume*100));
                            break;
                        case SDLK_v:
                            chip.volume += 0.1f;
                            if (chip.volume > 2.0f) chip.volume = 2.0f;
                            printf("Volume: %d%%\n", (int)(chip.volume*100));
                            break;
                    }
                }

                switch (evnt.key.keysym.sym) {
                    case SDLK_a: keys[0] = pressed; break;
                    case SDLK_s: keys[1] = pressed; break;
                    case SDLK_d: keys[2] = pressed; break;
                    case SDLK_f: keys[3] = pressed; break;
                    case SDLK_g: keys[4] = pressed; break;
                    case SDLK_h: keys[5] = pressed; break;
                    case SDLK_j: keys[6] = pressed; break;
                    case SDLK_w: keys[7] = pressed; break;
                    case SDLK_e: keys[8] = pressed; break;
                    case SDLK_t: keys[9] = pressed; break;
                    case SDLK_y: keys[10] = pressed; break;
                    case SDLK_u: keys[11] = pressed; break;
                }
            }

        }

        for (int i = 0; i < 12; i++) {
            int midi = base_note + key_to_midi[i];
            key_active_midi[i] = midi;
            float freq = getNote(midi);

            if (keys[i] && !prev_keys[i]) {
                int v = find_voice(voice_active);
                if (v == -1) {
                    v = 0;
                    for (int j = 0; j < 4; j++) {
                        if (chip.voices[j].env.state == ENV_RELEASE) { 
                            v = j;
                            break;
                        } else if (chip.voices[j].env.state == ENV_SUSTAIN && chip.voices[j].env.value <= 0.5f) {
                            v = j;
                            break;
                        }
                    }
                }

                Voice *voice = &chip.voices[v];
                voice->osc.frequency = freq;
                voice_note_on(voice);
                voice_note[v] = midi;
                voice_active[v] = true;
            }

            if (!keys[i] && prev_keys[i]) {
                int release_midi = key_active_midi[i];
                for (int v = 0; v < 4; v++) {
                    if (voice_active[v] && voice_note[v] == release_midi) {
                        voice_note_off(&chip.voices[v]);
                        voice_active[v] = false;
                        break;
                    }
                }
            }
            prev_keys[i] = keys[i];
        }

        SDL_SetRenderDrawColor(renderer, 0, 100, 150, 255);
        SDL_RenderClear(renderer);
        draw_piano(renderer, (WINDOW_SIZE-518)/2, (WINDOW_SIZE-KEY_HEIGHT)/2, keys);
        SDL_RenderPresent(renderer);
    }

    SDL_CloseAudioDevice(dev);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

