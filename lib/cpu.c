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

static void fetch_data() {
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;

    switch(ctx.cur_inst->mode) {
        case AM_IMP: 
            return;

        // Read data from register 1
        case AM_R:
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_1);
            return;

        // Read data from 8-bit value
        case AM_R_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        // Read data from 16-bit value
        case AM_D16: {
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            ctx.fetched_data = lo | (hi << 8);
            ctx.regs.pc += 2;
            return;
        }

        default:
            printf("Unknown Addressing Mode! %d\n", ctx.cur_inst->mode);
            exit(-7);
            return;
    }
}

static void execute() {
    printf("\tNot executing yet...\n");
}

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
