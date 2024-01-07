#ifndef __CPU_H__
#define __CPU_H__

#include <common.h>
#include <instructions.h>

typedef struct {
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u16 pc;                 // program counter
    u16 sp;                 // stack pointer
} cpu_registers;

typedef struct {
    cpu_registers regs;
    u16 fetched_data;       // currently fetched data
    u16 mem_dest;           // memory destination
    bool dest_is_mem;       // true if destination is memory location
    u8 cur_opcode;          // current operation code
    instruction *cur_inst;  // current instruction

    bool halted;
    bool stepping;
} cpu_context;

void cpu_init();
bool cpu_step();

#endif /* __CPU_H__ */
