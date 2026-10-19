#include "audio.h"

#include <math.h>

static constexpr int SAMPLE_RATE = 48000;
static constexpr float TONE_HZ = 440.0f;
static constexpr float AMPLITUDE = 0.15f;
static constexpr int CHUNK_SIZE = 1024;

static void SDLCALL beep_callback(void *userdata,
    SDL_AudioStream *stream,
    const int additional_amount,
    const int total_amount)
{
    (void) total_amount;
    chip8_beep *beep = userdata;
    float samples[CHUNK_SIZE];

    // Additional bytes of audio data requested by SDL
    int remaining = additional_amount;

    while (remaining > 0) {
        int count = remaining / (int)sizeof(float);
        if (count > CHUNK_SIZE)
            count = CHUNK_SIZE;
        if (count == 0)
            break;

        const bool playing = beep->playing;

        for (int i = 0; i < count; i++) {
            if (playing) {
                samples[i] = beep->phase < 0.5 ? AMPLITUDE : -AMPLITUDE;

                beep->phase += TONE_HZ / SAMPLE_RATE;
                if (beep->phase >= 1.0f)
                    beep->phase -= 1.0f;
            }
            else {
                samples[i] = 0.0f;
                beep->phase = 0.0f;
            }
        }

        const int bytes = (int)sizeof(float) * count;

        if (!SDL_PutAudioStreamData(stream, samples, bytes)) {
            SDL_Log("Audio write failed: %s", SDL_GetError());
            return;
        }

        remaining -= bytes;
    }
}

bool beep_init(chip8_beep *beep) {
    *beep = (chip8_beep) {0};
    beep->playing = false;

    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_F32,
        .channels = 1,
        .freq = SAMPLE_RATE
    };

    beep->stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        beep_callback,
        beep);

    if (!beep->stream) {
        SDL_Log("Could not open audio: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ResumeAudioStreamDevice(beep->stream)) {
        SDL_Log("Could not resume audio: %s", SDL_GetError());
        return false;
    }

    return true;
}

void beep_destroy(chip8_beep *beep) {
    if (beep->stream) {
        SDL_DestroyAudioStream(beep->stream);
        beep->stream = nullptr;
    }
}
