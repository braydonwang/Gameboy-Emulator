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
    u16 pc;                  // program counter
    u16 sp;                  // stack pointer
} cpu_registers;

typedef struct {
    cpu_registers regs;
    u16 fetched_data;        // currently fetched data
    u16 mem_dest;            // memory destination
    bool dest_is_mem;        // true if destination is memory location
    u8 cur_opcode;           // current operation code
    instruction *cur_inst;   // current instruction

    bool halted;
    bool stepping;

    bool int_master_enabled; // true if master interrupt is enabled
    bool enabling_ime;
    u8 ie_register;          // interrupt enable register
    u8 int_flags;

} cpu_context;

cpu_registers *cpu_get_regs();

void cpu_init();
bool cpu_step();

u16 cpu_read_reg(reg_type rt);
void cpu_set_reg(reg_type rt, u16 val);

u8 cpu_read_reg8(reg_type rt);
void cpu_set_reg8(reg_type rt, u8 val);

typedef void (*IN_PROC)(cpu_context *);

IN_PROC inst_get_processor(in_type type);

#define CPU_FLAG_Z BIT(ctx->regs.f, 7)
#define CPU_FLAG_C BIT(ctx->regs.f, 4)

typedef void (*IN_PROC)(cpu_context *);

IN_PROC inst_get_processor(in_type type);

#define CPU_FLAG_Z BIT(ctx->regs.f, 7)
#define CPU_FLAG_N BIT(ctx->regs.f, 6)
#define CPU_FLAG_H BIT(ctx->regs.f, 5)
#define CPU_FLAG_C BIT(ctx->regs.f, 4)

u8 cpu_get_ie_register();
void cpu_set_ie_register(u8 n);

u8 cpu_get_int_flags();
void cpu_set_int_flags(u8 value);

void inst_to_str(cpu_context *ctx, char *str);

#endif /* __CPU_H__ */
