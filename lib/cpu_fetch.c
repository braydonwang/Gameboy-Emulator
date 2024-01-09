#include <cpu.h>
#include <bus.h>
#include <emu.h>

extern cpu_context ctx;

void fetch_data() {
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
        case AM_R_D16: 
        case AM_D16: {
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            ctx.fetched_data = lo | (hi << 8);
            ctx.regs.pc += 2;
        } return;

        // Load register into a memory region
        case AM_MR_R: 
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.mem_dest = true;

            // instruction LDH C, (A)
            if (ctx.cur_inst->reg_1 == RT_C) { // carry flag register
                ctx.mem_dest |= 0xFF00; // write to C the MSO of 0xFF00
            }
            
            return;
        
        // Load memory region into a register
        case AM_R_MR: {
            u16 addr = cpu_read_reg(ctx.cur_inst->reg_2);

            if (ctx.cur_inst->reg_1 == RT_C) { // carry flag register
                addr |= 0xFF00; // write to C the MSO of 0xFF00
            }

            ctx.fetched_data = bus_read(addr);
            emu_cycles(1);

        } return;

        // Loading address of HL register then increment by 1
        case AM_R_HLI:
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
            return;

        // Loading address of HL register then decrement by 1
        case AM_R_HLD:
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
            return;

        // Load register value into HL register
        case AM_HLI_R:
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_2));
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
            return;
        
        // Load register value into HL register
        case AM_HLD_R:
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_2));
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
            return;

        // Move A8 into register
        case AM_R_A8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        // Move register into A8
        case AM_A8_R:
            ctx.mem_dest = bus_read(ctx.regs.pc) | 0xFF00;
            ctx.dest_is_mem = true;
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        // Special case - Load HL and SP, increment by r8
        case AM_HL_SPR:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        
        // 
        case AM_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        
        // Loading register into a 16 bit address
        case AM_A16_R: 
        case AM_D16_R: {
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            ctx.fetched_data = lo | (hi << 8);
            ctx.dest_is_mem = true;

            ctx.regs.pc += 2;
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            
        } return;

        // Load D8 into memory register
        case AM_MR_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            return;
        
        // Load into memory register
        case AM_MR:
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_1));
            emu_cycles(1);
            return;

        case AM_R_A16: {
            u16 lo = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            u16 addr = lo | (hi << 8);

            ctx.regs.pc += 2;
            ctx.fetched_data = bus_read(addr);
            emu_cycles(1);
        } return;

        default:
            printf("Unknown Addressing Mode! %d\n", ctx.cur_inst->mode);
            exit(-7);
            return;
    }
}