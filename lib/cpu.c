#include <cpu.h>
#include <bus.h>
#include <emu.h>

cpu_context ctx = {0};

void cpu_init() {
    ctx.regs.pc = 0x100;
}

static void fetch_instruction() {
    // Read op code and increment program counter
    ctx.cur_opcode = bus_read(ctx.regs.pc++);
    // Get current instruction based on op code
    ctx.cur_inst = instruction_by_opcode(ctx.cur_opcode);

    if (ctx.cur_inst == NULL) {
        printf("Unknown Instruction! %02X\n", ctx.cur_opcode);
        exit(-7);
    }
}

static void execute() {
    printf("\tNot executing yet...\n");
}

void fetch_data();

bool cpu_step() {
    if (!ctx.halted) {
        u16 pc = ctx.regs.pc;
        fetch_instruction();
        fetch_data();
        printf("Executing Instruction: %02X   PC: %04X\n", ctx.cur_opcode, pc);
        execute();
    }

    return true;
}
