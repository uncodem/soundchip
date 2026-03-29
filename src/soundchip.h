#ifndef SOUNDCHIP_H_
#define SOUNDCHIP_H_

#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_VOLUME 0.1f
#define DEFAULT_FREQ 440.0f
#define EPS 1e-6f

typedef struct {
    float phase;
    float frequency;
    float sample_rate;
    float noise_value;
    float integrator_state;
    float pulse_width;
} Oscillator;

Oscillator new_Oscillator(int sample_rate, float frequency);

float osc_sine(Oscillator *osc);
float osc_square(Oscillator *osc);
float osc_triangle(Oscillator *osc);
float osc_noise(Oscillator *osc);

typedef enum {
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_NOISE
} WaveType;

typedef enum {
    ENV_IDLE,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvState;

typedef struct {
    float attack;
    float decay;
    float sustain;
    float release;

    float value;
    EnvState state;
} Envelope;

typedef enum {
    LFO_PITCH,
    LFO_VOLUME,
    LFO_PWM
} LFOTarget;

/* LFO Offset is in hz */
typedef struct {
    Oscillator osc;
    float depth;
} LFO;

typedef struct {
    Oscillator osc;
    WaveType wave;
    float volume;
    bool enabled;
    Envelope env;
    LFO lfo;
    LFOTarget lfo_target;
} Voice;

void voice_note_on(Voice *v);
void voice_note_off(Voice *v);

typedef struct {
    Voice voices[4];
    float volume;
} SoundChip;

SoundChip new_SoundChip(int sample_rate);
float soundchip_sample(SoundChip *chip); 
void soundchip_generate(SoundChip *chip, uint8_t *stream, int len);

float getNote(int n);

#endif
