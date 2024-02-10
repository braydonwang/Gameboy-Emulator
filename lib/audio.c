#include <audio.h>
#include <common.h>
#include <SDL2/SDL.h>

#define AUDIO_CONVERT_SAMPLE_FROM_U8(X, fvol) ((fvol) * (X) * (1 / 255.0f))

typedef struct {
    SDL_AudioDeviceID dev;
    SDL_AudioSpec spec;
    u8* buffer;
    bool ready;
    float volume;
} audio_ctx;

typedef struct {
    u32 frequency;
    u32 freq_counter;
    u32 divisor;
    u32 frames;
    u8* data;
    u8* end;
    u8* position;
} audio_buffer;

static audio_ctx ctx;
static audio_buffer buffer;

void audio_init() {
    ctx.ready = false;
    ctx.volume = 0.7;

    SDL_AudioSpec want;
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 2048 * 2;
    want.callback = NULL;
    want.userdata = &ctx;

    ctx.dev = SDL_OpenAudioDevice(NULL, 0, &want, &ctx.spec, 0);
    ctx.buffer = calloc(1, ctx.spec.size);
}

void render_audio() {
    size_t src_frames = (buffer.position - buffer.data) / 2;
    size_t max_dst_frames = ctx.spec.size / (sizeof(float) * 2);
    size_t frames = MIN(src_frames, max_dst_frames);

    u8* src = buffer.data;
    float* dst = (float*)ctx.buffer;
    float* dst_end = dst + frames * 2;
    float volume = ctx.volume;

    size_t i;
    for (i = 0; i < frames; i++) {
        *dst++ = AUDIO_CONVERT_SAMPLE_FROM_U8(*src++, volume);
        *dst++ = AUDIO_CONVERT_SAMPLE_FROM_U8(*src++, volume);
    }

    u32 queued_size = SDL_GetQueuedAudioSize(ctx.dev);
    if (queued_size < 5 * ctx.spec.size) {
        u32 buffer_size = (u8*)dst_end - (u8*)ctx.buffer;
        SDL_QueueAudio(ctx.dev, ctx.buffer, buffer_size);
        // HOOK(audio_add_buffer, queued_size, queued_size + buffer_size);
        queued_size += buffer_size;
    }
    if (!ctx.ready && queued_size >= 2 * ctx.spec.size) {
        // HOOK(audio_buffer_ready, queued_size);
        ctx.ready = true;
        SDL_PauseAudioDevice(ctx.dev, 0);
    }
}
