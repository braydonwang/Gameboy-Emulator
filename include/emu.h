#ifndef __EMU_H__
#define __EMU_H__

#include <common.h>

typedef struct {
    bool paused;
    bool runnning;
    u64 ticks;
} emu_context;

int emu_run(int argc, char **argv);

emu_context *emu_get_context();

#endif /* __EMU_H__ */
