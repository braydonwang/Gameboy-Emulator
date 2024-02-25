#include <audio.h>
#include <common.h>
#include <emu.h>

#include <SDL2/SDL.h>

#define AUDIO_CONVERT_SAMPLE_FROM_U8(X, fvol) ((fvol) * (X) * (1 / 255.0f))
#define DIV_CEIL(numer, denom) (((numer) + (denom) - 1) / (denom))
#define CPU_TICKS_PER_SECOND 4194304
#define APU_TICKS_PER_SECOND 2097152
#define SOUND_OUTPUT_COUNT 2
#define SOUND_OUTPUT_MAX_VOLUME 7
#define FRAME_SEQUENCER_TICKS 8192
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define VALUE_WRAPPED(X, MAX) \
  (UNLIKELY((X) >= (MAX) ? ((X) -= (MAX), true) : false))
#define NEXT_MODULO(value, mod) ((mod) - (value) % (mod))

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

#define CHANNEL1 apu.channel[APU_CHANNEL1]
#define CHANNEL2 apu.channel[APU_CHANNEL2]
#define CHANNEL3 apu.channel[APU_CHANNEL3]
#define CHANNEL4 apu.channel[APU_CHANNEL4]

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
        queued_size += buffer_size;
    }
    if (!ctx.ready && queued_size >= 2 * ctx.spec.size) {
        ctx.ready = true;
        SDL_PauseAudioDevice(ctx.dev, 0);
    }
}

void audio_handle_event(u32 event) {
    if (event & EMULATOR_EVENT_AUDIO_BUFFER_FULL) {
        render_audio();
    }
}

void write_square_wave_period(Channel* channel, SquareWave* square) {
  square->period = ((2047 + 1) - channel->frequency) * 4;
  // HOOK(write_square_wave_period_info_iii, channel->frequency, square->ticks,
  //      square->period);
}

u16 calculate_sweep_frequency() {
  u16 f = apu.sweep.frequency;
  if (apu.sweep.direction == SWEEP_DIRECTION_ADDITION) {
    return f + (f >> apu.sweep.shift);
  } else {
    apu.sweep.calculated_subtract = true;
    return f - (f >> apu.sweep.shift);
  }
}

void update_sweep() {
  if (!(CHANNEL1.status && apu.sweep.enabled)) {
    return;
  }

  u8 period = apu.sweep.period;
  if (--apu.sweep.timer == 0) {
    if (period) {
      apu.sweep.timer = period;
      u16 new_frequency = calculate_sweep_frequency();
      if (new_frequency > 2047) {
        CHANNEL1.status = false;
      } else {
        if (apu.sweep.shift) {
          apu.sweep.frequency = CHANNEL1.frequency = new_frequency;
          write_square_wave_period(&CHANNEL1, &CHANNEL1.square_wave);
        }

        /* Perform another overflow check. */
        if (UNLIKELY(calculate_sweep_frequency() > 2047)) {
          CHANNEL1.status = false;
        }
      }
    } else {
      apu.sweep.timer = 8;
    }
  }
}

void update_lengths() {
  int i;
  for (i = 0; i < APU_CHANNEL_COUNT; ++i) {
    Channel* channel = &apu.channel[i];
    if (channel->length_enabled && channel->length > 0) {
      if (--channel->length == 0) {
        channel->status = false;
      }
    }
  }
}

void update_envelopes() {
  int i;
  for (i = 0; i < APU_CHANNEL_COUNT; ++i) {
    Envelope* envelope = &apu.channel[i].envelope;
    if (envelope->period) {
      if (envelope->automatic && --envelope->timer == 0) {
        envelope->timer = envelope->period;
        u8 delta = envelope->direction == ENVELOPE_ATTENUATE ? -1 : 1;
        u8 volume = envelope->volume + delta;
        if (volume < 15) {
          envelope->volume = volume;
        } else {
          envelope->automatic = false;
        }
      }
    } else {
      envelope->timer = 8;
    }
  }
}

u32 get_gb_frames_until_next_resampled_frame() {
  u32 result = 0;
  u32 counter = buffer.freq_counter;
  while (!VALUE_WRAPPED(counter, APU_TICKS_PER_SECOND)) {
    counter += buffer.frequency;
    result++;
  }
  return result;
}

#define CHANNELX_SAMPLE(channel, sample) \
  (-(sample) & (channel)->envelope.volume)

static void update_square_wave(Channel* channel, u32 total_frames) {
  static u8 duty[WAVE_DUTY_COUNT][8] =
      {[WAVE_DUTY_12_5] = {0, 0, 0, 0, 0, 0, 0, 1},
       [WAVE_DUTY_25] = {1, 0, 0, 0, 0, 0, 0, 1},
       [WAVE_DUTY_50] = {1, 0, 0, 0, 0, 1, 1, 1},
       [WAVE_DUTY_75] = {0, 1, 1, 1, 1, 1, 1, 0}};
  SquareWave* square = &channel->square_wave;
  if (channel->status) {
    while (total_frames) {
      u32 frames = square->ticks / 2;
      u8 sample = CHANNELX_SAMPLE(channel, square->sample);
      if (frames <= total_frames) {
        square->ticks = square->period;
        square->position = (square->position + 1) % 8;
        square->sample = duty[square->duty][square->position];
      } else {
        frames = total_frames;
        square->ticks -= frames * 2;
      }
      channel->accumulator += sample * frames;
      total_frames -= frames;
    }
  }
}

void update_wave(u32 apu_ticks, u32 total_frames) {
  if (CHANNEL3.status) {
    while (total_frames) {
      u32 frames = apu.wave.ticks / 2;
      u8 sample = apu.wave.sample_data >> apu.wave.volume_shift;
      if (frames <= total_frames) {
        apu.wave.position = (apu.wave.position + 1) % 32;
        apu.wave.sample_time = apu_ticks + apu.wave.ticks;
        u8 byte = apu.wave.ram[apu.wave.position >> 1];
        if ((apu.wave.position & 1) == 0) {
          apu.wave.sample_data = byte >> 4;
        } else {
          apu.wave.sample_data = byte & 0x0f;
        }
        apu.wave.ticks = apu.wave.period;
        // HOOK(wave_update_position_iii, WAVE.position, WAVE.sample_data,
        //      WAVE.sample_time);
      } else {
        frames = total_frames;
        apu.wave.ticks -= frames * 2;
      }
      apu_ticks += frames * 2;
      CHANNEL3.accumulator += sample * frames;
      total_frames -= frames;
    }
  }
}

void update_noise(u32 total_frames) {
  if (CHANNEL4.status) {
    while (total_frames) {
      u32 frames = apu.noise.ticks / 2;
      u8 sample = CHANNELX_SAMPLE(&CHANNEL4, apu.noise.sample);
      if (apu.noise.clock_shift <= 13) {
        if (frames <= total_frames) {
          u16 bit = (apu.noise.lfsr ^ (apu.noise.lfsr >> 1)) & 1;
          if (apu.noise.lfsr_width == LFSR_WIDTH_7) {
            apu.noise.lfsr = ((apu.noise.lfsr >> 1) & ~0x40) | (bit << 6);
          } else {
            apu.noise.lfsr = ((apu.noise.lfsr >> 1) & ~0x4000) | (bit << 14);
          }
          apu.noise.sample = ~apu.noise.lfsr & 1;
          apu.noise.ticks = apu.noise.period;
        } else {
          frames = total_frames;
          apu.noise.ticks -= frames * 2;
        }
      } else {
        frames = total_frames;
      }
      CHANNEL4.accumulator += sample * frames;
      total_frames -= frames;
    }
  }
}

void apu_update_channels(u32 total_frames) {
  while (total_frames) {
    u32 frames = get_gb_frames_until_next_resampled_frame();
    frames = MIN(frames, total_frames);
    update_square_wave(&CHANNEL1, frames);
    update_square_wave(&CHANNEL2, frames);
    update_wave(apu.sync_ticks, frames);
    update_noise(frames);
    write_audio_frame(frames);
    apu.sync_ticks += frames * 2;
    total_frames -= frames;
  }
}

void apu_update(u32 total_ticks) {
  while (total_ticks) {
    u64 next_seq_ticks = NEXT_MODULO(apu.sync_ticks, FRAME_SEQUENCER_TICKS);
    if (next_seq_ticks == FRAME_SEQUENCER_TICKS) {
      apu.frame = (apu.frame + 1) % 8;
      switch (apu.frame) {
        case 2: case 6: update_sweep();
        case 0: case 4: update_lengths(); break;
        case 7: update_envelopes(); break;
      }
    }
    u64 ticks = MIN(next_seq_ticks, total_ticks);
    apu_update_channels(ticks / 2);
    total_ticks -= ticks;
  }
}

void write_audio_frame(u32 gb_frames) {
  int i, j;
  buffer.divisor += gb_frames;
  buffer.freq_counter += buffer.frequency * gb_frames;
  if (VALUE_WRAPPED(buffer.freq_counter, APU_TICKS_PER_SECOND)) {
    for (i = 0; i < SOUND_OUTPUT_COUNT; ++i) {
      u32 accumulator = 0;
      for (j = 0; j < APU_CHANNEL_COUNT; ++j) {
        accumulator += apu.channel[j].accumulator * apu.so_output[j][i];
      }
      accumulator *= (apu.so_volume[i] + 1) * 16;
      accumulator /= ((SOUND_OUTPUT_MAX_VOLUME + 1) * APU_CHANNEL_COUNT);
      *(buffer.position)++ = accumulator / buffer.divisor;
    }
    for (j = 0; j < APU_CHANNEL_COUNT; ++j) {
      apu.channel[j].accumulator = 0;
    }
    buffer.divisor = 0;
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
        printf("HI");
    } while (!(event & (EMULATOR_EVENT_UNTIL_TICKS | EMULATOR_EVENT_BREAKPOINT | EMULATOR_EVENT_INVALID_OPCODE)));

    return event;
}

u32 audio_run_ms(double delta_ms) {
  u64 delta_ticks = (u64)(delta_ms * CPU_TICKS_PER_SECOND / 1000);
  u64 until_ticks = emu_get_context()->ticks + delta_ticks;
  u32 event = audio_run_until_ticks(until_ticks);
  return event;
}
