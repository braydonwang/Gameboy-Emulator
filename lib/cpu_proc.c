#include <cpu.h>
#include <emu.h>

// Processes CPU instructionss...

// None instruction
static void proc_none(cpu_context *ctx) {
    printf("INVALID INSTRUCTION!\n");
    exit(-7);
}

// Nop instruction
static void proc_nop(cpu_context *ctx) {

}

// Disable interrupts instruction
static void proc_di(cpu_context *ctx) {
    ctx->int_master_enabled = false;
}

// Load instruction
static void proc_ld(cpu_context *ctx) {
    //TODO...
}

// Sets flag bits
void cpu_set_flags(cpu_context *ctx, char z, char n, char h, char c) {
    if (z != -1) {
        BIT_SET(ctx->regs.f, 7, z);
    }

    if (n != -1) {
        BIT_SET(ctx->regs.f, 6, n);
    }

    if (h != -1) {
        BIT_SET(ctx->regs.f, 5, h);
    }

    if (c != -1) {
        BIT_SET(ctx->regs.f, 4, c);
    }
}

// Xor instruction
static void proc_xor(cpu_context *ctx) {
    ctx->regs.a ^= ctx->fetched_data & 0xFF;    // XOR reg A and fetched data
    cpu_set_flags(ctx, ctx->regs.a, 0, 0, 0);   // sets flags to (Z 0 0 0)
}

// Returns bool representing if condition is satisfied
static bool check_cond(cpu_context *ctx) {
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;

    switch(ctx->cur_inst->cond) {
        case CT_NONE: return true;
        case CT_C: return c;
        case CT_NC: return !c;
        case CT_Z: return z;
        case CT_NZ: return !z;
    }

    return false;
}

// Jump instruction
static void proc_jp(cpu_context *ctx) {
    if (check_cond(ctx)) {
        ctx->regs.pc = ctx->fetched_data;       // set pc to jump address
        emu_cycles(1);
    }
}

static IN_PROC processors[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_LD] = proc_ld,
    [IN_JP] = proc_jp,
    [IN_DI] = proc_di,
    [IN_XOR] = proc_xor
};

// Run processor for each instruction type
IN_PROC inst_get_processor(in_type type) {
    return processors[type];
}
