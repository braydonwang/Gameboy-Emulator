#ifndef __EMU_H__
#define __EMU_H__

#include <common.h>

typedef struct {
    bool paused;
    bool running;
    u64 ticks;
} emu_context;

int emu_run(int argc, char **argv);

emu_context *emu_get_context();

void emu_cycles(int cpu_cycles);

#endif /* __EMU_H__ */
