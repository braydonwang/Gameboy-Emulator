#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <common.h>

enum {
    EMULATOR_EVENT_NEW_FRAME = 0x1,
    EMULATOR_EVENT_AUDIO_BUFFER_FULL = 0x2,
    EMULATOR_EVENT_UNTIL_TICKS = 0x4,
    EMULATOR_EVENT_BREAKPOINT = 0x8,
    EMULATOR_EVENT_INVALID_OPCODE = 0x10,
};

enum {
  APU_CHANNEL1,
  APU_CHANNEL2,
  APU_CHANNEL3,
  APU_CHANNEL4,
  APU_CHANNEL_COUNT,
};

void audio_init();
void render_audio();
void audio_handle_event();

u32 audio_run_until(u64 until_ticks);
u32 audio_run_until_ticks(u64 ticks);

void apu_update(u32 total_ticks);
void apu_synchronize();
void write_audio_frame(u32 gb_frames);

u32 audio_run_ms(double delta_ms);

#endif /* __AUDIO_H__ */
