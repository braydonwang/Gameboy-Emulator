#include <audio.h>
#include <common.h>
#include <emu.h>

#include <SDL2/SDL.h>

#define AUDIO_CONVERT_SAMPLE_FROM_U8(X, fvol) ((fvol) * (X) * (1 / 255.0f))
#define DIV_CEIL(numer, denom) (((numer) + (denom) - 1) / (denom))
#define CPU_TICKS_PER_SECOND 4194304

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

typedef enum {
  SWEEP_DIRECTION_ADDITION = 0,
  SWEEP_DIRECTION_SUBTRACTION = 1,
} SweepDirection;

typedef struct {
  u8 period;
  SweepDirection direction;
  u8 shift;
  u16 frequency;
  u8 timer;
  bool enabled;
  bool calculated_subtract;
} Sweep;

typedef enum {
  WAVE_VOLUME_MUTE = 0,
  WAVE_VOLUME_100 = 1,
  WAVE_VOLUME_50 = 2,
  WAVE_VOLUME_25 = 3,
  WAVE_VOLUME_COUNT,
} WaveVolume;

typedef struct {
  WaveVolume volume;
  u8 volume_shift;
  u8 ram[16];
  u64 sample_time;
  u8 sample_data;
  u32 period;
  u8 position;      
  u32 ticks;         
  bool playing;      
} Wave;

typedef enum {
  LFSR_WIDTH_15 = 0,
  LFSR_WIDTH_7 = 1,
} LfsrWidth;

typedef struct {
  u8 clock_shift;
  LfsrWidth lfsr_width;
  u8 divisor;
  u8 sample;
  u16 lfsr;
  u32 period;
  u32 ticks; 
} Noise;

typedef enum {
  ENVELOPE_ATTENUATE = 0,
  ENVELOPE_AMPLIFY = 1,
} EnvelopeDirection;

typedef enum {
  WAVE_DUTY_12_5 = 0,
  WAVE_DUTY_25 = 1,
  WAVE_DUTY_50 = 2,
  WAVE_DUTY_75 = 3,
  WAVE_DUTY_COUNT,
} WaveDuty;

typedef struct {
  u8 initial_volume;
  EnvelopeDirection direction;
  u8 period;
  u8 volume;
  u32 timer;
  bool automatic;
  u8 zombie_step;
} Envelope;

/* Channel 1 and 2 */
typedef struct {
  WaveDuty duty;
  u8 sample;
  u32 period;
  u8 position;
  u32 ticks;
} SquareWave;

typedef struct {
  SquareWave square_wave;
  Envelope envelope;
  u16 frequency;
  u16 length;
  bool length_enabled;
  bool dac_enabled;
  bool status;
  u32 accumulator;
} Channel;

typedef struct {
    u8 so_volume[2];
    bool so_output[5][2];
    bool enabled;
    Sweep sweep;
    Wave wave;
    Noise noise;
    Channel channel[APU_CHANNEL_COUNT];
    u8 frame;
    u64 sync_ticks;
    bool initialized;
} apu_ctx;

static audio_ctx ctx;
static audio_buffer buffer;
static apu_ctx apu;

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

void audio_handle_event(u32 event) {
    if (event & EMULATOR_EVENT_AUDIO_BUFFER_FULL) {
        host_render_audio();
    }
}

void apu_update(u32 total_ticks) {
  while (total_ticks) {
    u64 next_seq_ticks = NEXT_MODULO(apu.sync_ticks, FRAME_SEQUENCER_TICKS);
    if (next_seq_ticks == FRAME_SEQUENCER_TICKS) {
      APU.frame = (APU.frame + 1) % FRAME_SEQUENCER_COUNT;
      switch (APU.frame) {
        case 2: case 6: update_sweep(e); /* Fallthrough. */
        case 0: case 4: update_lengths(e); break;
        case 7: update_envelopes(e); break;
      }
    }
    Ticks ticks = MIN(next_seq_ticks, total_ticks);
    apu_update_channels(e, ticks / APU_TICKS);
    total_ticks -= ticks;
  }
}

void write_audio_frame(u32 gb_frames) {
  int i, j;
  AudioBuffer* buffer = &e->audio_buffer;
  buffer->divisor += gb_frames;
  buffer->freq_counter += buffer->frequency * gb_frames;
  if (VALUE_WRAPPED(buffer->freq_counter, APU_TICKS_PER_SECOND)) {
    for (i = 0; i < SOUND_OUTPUT_COUNT; ++i) {
      u32 accumulator = 0;
      for (j = 0; j < APU_CHANNEL_COUNT; ++j) {
        if (!e->config.disable_sound[j]) {
          accumulator += APU.channel[j].accumulator * APU.so_output[j][i];
        }
      }
      accumulator *= (APU.so_volume[i] + 1) * 16; /* 4bit -> 8bit samples. */
      accumulator /= ((SOUND_OUTPUT_MAX_VOLUME + 1) * APU_CHANNEL_COUNT);
      *buffer->position++ = accumulator / buffer->divisor;
    }
    for (j = 0; j < APU_CHANNEL_COUNT; ++j) {
      APU.channel[j].accumulator = 0;
    }
    buffer->divisor = 0;
  }
}

void apu_synchronize() {
  if (emu_get_context()->ticks > apu.sync_ticks) {
    u32 ticks = emu_get_context()->ticks - apu.sync_ticks;
    if (apu.enabled) {
      apu_update(ticks);
    } else {
      for (; ticks; ticks -= 2) {
        write_audio_frame(1);
      }
      apu.sync_ticks = emu_get_context()->ticks;
    }
  }
}

u32 audio_run_until(u64 until_ticks) {
    if (emu_get_context()->event & EMULATOR_EVENT_AUDIO_BUFFER_FULL) {
        buffer.position = buffer.data;
    }
    emu_get_context()->event = 0;

    u64 frames_left = buffer.frames - (buffer.position - buffer.data) / 2;
    u64 max_audio_ticks = apu.sync_ticks + (u32)DIV_CEIL(frames_left * 4194304, buffer.frequency);

    u64 check_ticks = MIN(until_ticks, max_audio_ticks);
    while (emu_get_context()->event == 0 && emu_get_context()->ticks < check_ticks) {
        emu_cycles(1);
    }
    if (emu_get_context()->ticks >= max_audio_ticks) {
        emu_get_context()->event |= EMULATOR_EVENT_AUDIO_BUFFER_FULL;
    }
    if (emu_get_context()->ticks >= until_ticks) {
        emu_get_context()->event |= EMULATOR_EVENT_UNTIL_TICKS;
    }
    apu_synchronize();
    return emu_get_context()->event;
}

u32 audio_run_until_ticks(u64 ticks) {
    u32 event;
    do {
        event = audio_run_until(ticks);
        audio_handle_event(event);
    } while (!(event & (EMULATOR_EVENT_UNTIL_TICKS | EMULATOR_EVENT_BREAKPOINT | EMULATOR_EVENT_INVALID_OPCODE)));

    return event;
}
