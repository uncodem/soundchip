#include <math.h>
#include <stdint.h>
#include "soundchip.h"

static float lfsr() {
    static uint16_t reg = 0x7fff;
    uint8_t lsb = reg & 0x01;

    uint8_t res = lsb;
    res ^= (reg >> 1) & 0x01;
    reg >>= 1;
    reg |= (res << 14);

    return lsb ? -1.0f : 1.0f;
}

static inline void osc_step(Oscillator *osc) {
    osc->phase += osc->frequency / osc->sample_rate;
    if (osc->phase >= 1.0f) osc->phase -= 1.0f;
}

float osc_sine(Oscillator *osc) {
    float ret = sinf(osc->phase * 2.0f * M_PI);
    osc_step(osc);
    return ret;
}

static float correction(float t, float dt) {
    if (t < dt) {
        t /= dt;
        return 2.0f * t - t*t - 1.0f;
    } else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t*t + 2.0f * t + 1.0f;
    } 
    return 0;
}

float osc_square(Oscillator *osc) {
    float t = osc->phase;
    float dt = osc->frequency / osc->sample_rate;

    float pw = osc->pulse_width;
    if (pw < 0.02f) pw = 0.02f;
    if (pw > 0.98f) pw = 0.98f;

    float ret = (t < pw) ? 1.0f : -1.0f;

    ret += correction(t, dt);

    float t2 = t + pw;
    if (t2 >= 1.0f) t2 -= 1.0f;

    ret -= correction(t2, dt);

    osc_step(osc);
    return ret;
}

float osc_triangle(Oscillator *osc) {
    float square_sample = osc_square(osc);
    float leak = 0.999f;

    osc->integrator_state = (leak * osc->integrator_state) + square_sample;

    float amplitude_scale = 4.0f * (osc->frequency / osc->sample_rate);

    return osc->integrator_state * amplitude_scale;
}

float osc_noise(Oscillator *osc) {
    osc->phase += osc->frequency / osc->sample_rate;
    if (osc->phase >= 1.0f) {
        osc->phase -= 1.0f;
        osc->noise_value = lfsr();
    }
    return osc->noise_value;
}

Oscillator new_Oscillator(int sample_rate, float frequency) {
    Oscillator osc = {0};
    osc.sample_rate = sample_rate;
    osc.frequency = frequency;
    osc.pulse_width = 0.5f;
    return osc;
}

static Voice new_voice(int sample_rate) {
    Voice ret = {0};
    ret.osc = new_Oscillator(sample_rate, DEFAULT_FREQ);
    ret.volume = DEFAULT_VOLUME;
    ret.wave = WAVE_SINE;

    ret.env = (Envelope){
        .attack = 0.01f,
        .decay = 0.1f,
        .sustain = 0.8f,
        .release = 0.2f,
        .value = 0.0f,
        .state = ENV_IDLE
    };

    ret.lfo = (LFO){
        .osc = new_Oscillator(sample_rate, 5.0f),
        .depth = 0.0f
    };

    return ret;
}

SoundChip new_SoundChip(int sample_rate) {
    SoundChip ret = {0};
    ret.volume = 1.0f;

    for (int i = 0; i < 4; i++) {
        ret.voices[i] = new_voice(sample_rate); 
    }

    /* Default values, user is expected to change */
    ret.voices[0].wave = WAVE_SINE;
    ret.voices[1].wave = WAVE_SQUARE;
    ret.voices[2].wave = WAVE_TRIANGLE;
    ret.voices[3].wave = WAVE_NOISE;

    return ret;
}

static float env_step(Envelope *e, float dt) {
    switch (e->state) {
        case ENV_ATTACK: {
            float attack = (e->attack < EPS) ? EPS : e->attack;
            e->value += dt / attack;
            if (e->value >= 1.0f) {
                e->value = 1.0f;
                e->state = ENV_DECAY;
            }
            break;
        }
        case ENV_DECAY: {
            float decay = (e->decay < EPS) ? EPS : e->decay;
            e->value -= dt * (1.0f - e->sustain) / decay;
            if (e->value <= e->sustain) {
                e->value = e->sustain;
                e->state = ENV_SUSTAIN;
            }
            break;
        }
        case ENV_SUSTAIN:
            break;
        case ENV_RELEASE: {
            float release = (e->release < EPS) ? EPS : e->release;
            e->value *= expf(-dt/release);
            // e->value -= dt / release;
            if (e->value <= 0.0f) {
                e->value = 0.0f;
                e->state = ENV_IDLE;
            }
            break;
        }
        default: break;
    }
    return e->value;
}

static float voice_sample(Voice *v) {
    float ret = 0.0f;

    float lfo = osc_sine(&v->lfo.osc) * v->lfo.depth;

    float base_freq = v->osc.frequency;
    float base_pw = v->osc.pulse_width;

    if (v->lfo_target == LFO_PITCH) v->osc.frequency += lfo;
    else if (v->lfo_target == LFO_PWM) v->osc.pulse_width += lfo;

    switch(v->wave) {
        case WAVE_SINE: ret = osc_sine(&v->osc); break;
        case WAVE_NOISE: ret = osc_noise(&v->osc); break;
        case WAVE_SQUARE: ret = osc_square(&v->osc); break;
        case WAVE_TRIANGLE: ret = osc_triangle(&v->osc); break;
    }

    v->osc.frequency = base_freq;
    v->osc.pulse_width = base_pw;

    float env = env_step(&v->env, 1.0f / v->osc.sample_rate);
    ret *= env;

    if (v->lfo_target == LFO_VOLUME) {
        ret *= (1.0f + lfo);
    }

    return ret;
}

void voice_note_on(Voice *v) {
    v->env.state = ENV_ATTACK;
}

void voice_note_off(Voice *v) {
    v->env.state = ENV_RELEASE;
}

float soundchip_sample(SoundChip *chip) {
    float ret = 0.0f;

    for (int i = 0; i < 4; i++) {
        Voice *v = &chip->voices[i];
        if (!v->enabled) continue;

        float s = voice_sample(v);
        ret += s*v->volume;
    }

    ret *= chip->volume;

    if (ret > 1.0f) ret = 1.0f;
    if (ret < -1.0f) ret = -1.0f;

    return ret;
}

void soundchip_generate(SoundChip *chip, uint8_t *stream, int len) {
    float *buffer = (float*)stream;
    int samples = len / sizeof(float);

    for (int i = 0; i < samples; i++) {
        buffer[i] = soundchip_sample(chip);
    }
}

float getNote(int n) {
    return 440.0f * powf(2, (n-69)/12.0f);
}

