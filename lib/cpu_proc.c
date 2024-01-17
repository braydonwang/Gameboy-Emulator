#include <cpu.h>
#include <emu.h>
#include <bus.h>
#include <stack.h>

/*
    Processing CPU Instructions
*/

// None instruction
static void proc_none(cpu_context *ctx) {
    printf("INVALID INSTRUCTION!\n");
    exit(-7);
}

// NOP instruction
static void proc_nop(cpu_context *ctx) {}

// Disable interrupts instruction
static void proc_di(cpu_context *ctx) {
    ctx->int_master_enabled = false;
}

// Enable interrupts instruction
static void proc_ei(cpu_context *ctx) {
    ctx->enabling_ime = true;
}

static bool is_16_bit(reg_type rt) {
    return rt >= RT_AF;
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

// Lookup table for register types
reg_type rt_lookup[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A
};

// Lookup the register type
reg_type decode_reg(u8 reg) {
    if (reg > 0b111) { // 0b111 for the last three bits
        return RT_NONE;
    }

    return rt_lookup[reg];
}

// Bitwise operations
// Decode second byte that comes after the CB instr
static void proc_cb(cpu_context *ctx) {      // reference prefix CB table
    u8 op = ctx->fetched_data;
    reg_type reg = decode_reg(op & 0b111);
    u8 bit = (op >> 3) & 0b111;              // mask last three bits
    u8 bit_op = (op >> 6) & 0b11;            // mask last two bits
    u8 reg_val = cpu_read_reg8(reg);

    emu_cycles(1);

    if (reg == RT_HL) {
        emu_cycles(2);
    }

    switch (bit_op) {
    case 1: // BIT
        cpu_set_flags(ctx, !(reg_val & (1 << bit)), 0, 1, -1);
        return;

    case 2: // RST - reset
        reg_val &= ~(1 << bit);
        cpu_set_reg8(reg, reg_val);
        return;

    case 3: // SET
        reg_val |= (1 << bit);
        cpu_set_reg8(reg, reg_val);
        return;
    }

    bool flagC = CPU_FLAG_C;

    switch (bit) {

        case 0: { // RLC rotate left, old bit 7 to the carry flag
            bool setC = false;
            u8 result = (reg_val << 1) & 0xFF;

            if ((reg_val & (1 << 7)) != 0) {
                result |= 1;
                setC = true;
            }
            cpu_set_reg8(reg, result);
            cpu_set_flags(ctx, result == 0, false, false, setC);
        } return;
    
        case 1: { // RRC - rotate right, carry is what used to be the LSB
            u8 old = reg_val;
            reg_val >>= 1;
            reg_val |= (old << 7);

            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, false, false, old & 1); // old & 1 - whether carry flag used
        } return;

        case 2: { // RL - rotate left
            u8 old = reg_val;
            reg_val <<= 1;
            reg_val |= flagC;

            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, false, false, !!(old & 0x80)); // !!(old & 0x80) - if high bit set
        } return;

        case 3: { // RR - rotate right
        u8 old = reg_val;
            reg_val >>= 1;
            reg_val |= (flagC << 7);

            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, false, false, old & 1);
        } return;

        case 4: { // SLA - shift left and carry, lsb set to 0
            u8 old = reg_val;
            reg_val <<= 1;

            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, false, false, !!(old & 0x80));
        } return;

        case 5: { // SRA - shift right and carry, MSB doesn't change
            u8 u = (int8_t) reg_val >> 1;
            cpu_set_reg8(reg, u);
            cpu_set_flags(ctx, !u, 0, 0, reg_val & 1);
        } return;

        case 6: { // SWAP - high nibble swap with low nibble
            reg_val = ((reg_val & 0xF0) >> 4) | ((reg_val & 0xF) << 4);
            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, reg_val == 0, false, false, false);
        } return;

        case 7: { // SRL - shift right and carry, msb set to 0
            u8 u = reg_val >> 1;
            cpu_set_reg8(reg, u);
            cpu_set_flags(ctx, !u, 0, 0, reg_val & 1);
        } return;

    }
    fprintf(stderr, "ERROR: INVALID CB: %02X", op);
    NO_IMPL
}

// RLCA (Rotate Left A Affect Carry) instruction
static void proc_rlca(cpu_context *ctx) {
    u8 u = ctx->regs.a;
    bool c = (u >> 7) & 1;
    u = (u << 1) | c;
    ctx->regs.a = u;

    cpu_set_flags(ctx, 0, 0, 0, c);
}

// RRCA (Rotate Right A Affect Carry) instruction
static void proc_rrca(cpu_context *ctx) {
    u8 b = ctx->regs.a & 1;
    ctx->regs.a >>= 1;
    ctx->regs.a |= (b << 7);

    cpu_set_flags(ctx, 0, 0, 0, b);
}

// RLA (Rotate Left A) instruction
static void proc_rla(cpu_context *ctx) {
    u8 u = ctx->regs.a;
    u8 cf = CPU_FLAG_C;
    u8 c = (u >> 7) & 1;

    ctx->regs.a = (u << 1) | cf;
    cpu_set_flags(ctx, 0, 0, 0, c);
}

// RRA (Rotate Right A) instruction
static void proc_rra(cpu_context *ctx) {
    u8 carry = CPU_FLAG_C;
    u8 new_c = ctx->regs.a & 1;

    ctx->regs.a >>= 1;
    ctx->regs.a |= (carry << 7);

    cpu_set_flags(ctx, 0, 0, 0, new_c);
}

// Stop instruction
static void proc_stop(cpu_context *ctx) {
    fprintf(stderr, "STOPPING!\n");
    NO_IMPL
}

// DAA (Decimal Adjust Accumulator) instruction
static void proc_daa(cpu_context *ctx) {
    u8 u = 0;
    int fc = 0;

    if (CPU_FLAG_H || (!CPU_FLAG_N && (ctx->regs.a & 0XF) > 9)) {
        u = 6;
    }

    if (CPU_FLAG_C || (!CPU_FLAG_N && ctx->regs.a > 0x99)) {
        u |= 0x60;
        fc = 1;
    }

    ctx->regs.a += CPU_FLAG_N ? -u : u;

    cpu_set_flags(ctx, ctx->regs.a == 0, -1, 0, fc);
}

// CPL (Complement Accumulator) instruction
static void proc_cpl(cpu_context *ctx) {
    ctx->regs.a = ~ctx->regs.a;
    cpu_set_flags(ctx, -1, 1, 1, -1);
}

// SCF (Set Carry Flag) instruction
static void proc_scf(cpu_context *ctx) {
    cpu_set_flags(ctx, -1, 0, 0, 1);
}

// CCF (Complement Carry Flag) instruction
static void proc_ccf(cpu_context *ctx) {
    cpu_set_flags(ctx, -1, 0, 0, CPU_FLAG_C ^ 1);  // invert carry flag
}

// Halt instruction
static void proc_halt(cpu_context *ctx) {
    ctx->halted = true;
}

// AND instruction
static void proc_and(cpu_context *ctx) {
    ctx->regs.a &= ctx->fetched_data;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 1, 0);
}

// XOR instruction
static void proc_xor(cpu_context *ctx) {
    ctx->regs.a ^= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

// OR instruction
static void proc_or(cpu_context *ctx) {
    ctx->regs.a |= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

// CP (Compare) instruction
static void proc_cp(cpu_context *ctx) {
    int n = (int) ctx->regs.a - (int) ctx->fetched_data;
    cpu_set_flags(ctx, n == 0, 1, 
        ((int) ctx->regs.a & 0x0F) - ((int) ctx->fetched_data & 0x0F) < 0, n < 0);
}

// Load instruction
static void proc_ld(cpu_context *ctx) {
    if (ctx->dest_is_mem) {
        if (is_16_bit(ctx->cur_inst->reg_2)) {   // in the form: LD (BC), A (and 16-bit register)
            emu_cycles(1);
            bus_write16(ctx->mem_dest, ctx->fetched_data);
        } else {
            bus_write(ctx->mem_dest, ctx->fetched_data);
        }

        emu_cycles(1);
        return;
    }

    if (ctx->cur_inst->mode == AM_HL_SPR) {   // special case
        u8 hflag = (cpu_read_reg(ctx->cur_inst->reg_2) & 0xF) + 
            (ctx->fetched_data & 0xF) >= 0x10;

        u8 cflag = (cpu_read_reg(ctx->cur_inst->reg_2) & 0xFF) + 
            (ctx->fetched_data & 0xFF) >= 0x100;

        cpu_set_flags(ctx, 0, 0, hflag, cflag);
        cpu_set_reg(ctx->cur_inst->reg_1, 
            cpu_read_reg(ctx->cur_inst->reg_2) + (char)ctx->fetched_data);

        return;
    }

    cpu_set_reg(ctx->cur_inst->reg_1, ctx->fetched_data);
}

// LDH (Load High) instruction
static void proc_ldh(cpu_context *ctx) {
    if (ctx->cur_inst->reg_1 == RT_A) {
        cpu_set_reg(ctx->cur_inst->reg_1, bus_read(0xFF00 | ctx->fetched_data));
    } else {
        bus_write(ctx->mem_dest, ctx->regs.a);
    }

    emu_cycles(1);
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

static void goto_addr(cpu_context *ctx, u16 addr, bool pushpc) {
    if (check_cond(ctx)) {                     // checking if conditional flag is met
        if (pushpc) {                          // push pc to stack
            emu_cycles(2);
            stack_push16(ctx->regs.pc);
        }
        ctx->regs.pc = addr;                   // set pc to address
        emu_cycles(1);
    }
}

// Jump instruction
static void proc_jp(cpu_context *ctx) {
    goto_addr(ctx, ctx->fetched_data, false);
}

// Jump relative instruction
static void proc_jr(cpu_context *ctx) {
    char rel = (char)(ctx->fetched_data & 0xFF);   // casting to char because relative jump may be negative
    u16 addr = ctx->regs.pc + rel;
    goto_addr(ctx, addr, false);
}

// Call instruction
static void proc_call(cpu_context *ctx) {
    goto_addr(ctx, ctx->fetched_data, true);
}

// Restart instruction
static void proc_rst(cpu_context *ctx) {
    goto_addr(ctx, ctx->cur_inst->param, true);
}

// Return instruction
static void proc_ret(cpu_context *ctx) {
    if (ctx->cur_inst->cond != CT_NONE) {
        emu_cycles(1);
    }

    if (check_cond(ctx)) {
        u16 lo = stack_pop();
        emu_cycles(1);
        u16 hi = stack_pop();
        emu_cycles(1);

        u16 n = (hi << 8) | lo;
        ctx->regs.pc = n;                   // set pc to value on stack
        emu_cycles(1);
    }
}

// Return interrupt instruction
static void proc_reti(cpu_context *ctx) {
    ctx->int_master_enabled = true;
    proc_ret(ctx);
}

// Pop instruction
static void proc_pop(cpu_context *ctx) {
    u16 lo = stack_pop();
    emu_cycles(1);
    u16 hi = stack_pop();
    emu_cycles(1);

    u16 n = (hi << 8) | lo;
    cpu_set_reg(ctx->cur_inst->reg_1, n);

    if (ctx->cur_inst->reg_1 == RT_AF) {
        cpu_set_reg(ctx->cur_inst->reg_1, n & 0xFFF0);
    }
}


// Push instruction
static void proc_push(cpu_context *ctx) {
    u16 hi = (cpu_read_reg(ctx->cur_inst->reg_1) >> 8) & 0xFF;
    emu_cycles(1);
    stack_push(hi);

    u16 lo = cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF;
    emu_cycles(1);
    stack_push(lo);

    emu_cycles(1);
}

// Inc instruction
static void proc_inc(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) + 1;

    if (is_16_bit(ctx->cur_inst->reg_1)) {          // 16 bit inc
        emu_cycles(1);
    }

    if (ctx->cur_inst->reg_1 == RT_HL && ctx->cur_inst->mode == AM_MR) {        // inc memory value at register address instead of register value
        val = bus_read(cpu_read_reg(RT_HL)) + 1;
        val &= 0xFF;
        bus_write(cpu_read_reg(RT_HL), val);
    } else{
        cpu_set_reg(ctx->cur_inst->reg_1, val);
        val = cpu_read_reg(ctx->cur_inst->reg_1);
    }

    if ((ctx->cur_opcode & 0x03) == 0x03) {         // 0x3 do not change flags
        return;
    }

    cpu_set_flags(ctx, val == 0, 0, (val & 0x0F) == 0, -1);
}

// Dec instruction
static void proc_dec(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) - 1;

    if (is_16_bit(ctx->cur_inst->reg_1)) {          // 16 bit dec
        emu_cycles(1);
    }

    if (ctx->cur_inst->reg_1 == RT_HL && ctx->cur_inst->mode == AM_MR) {        // dec memory value at register address instead of register value
        val = bus_read(cpu_read_reg(RT_HL)) - 1;
        bus_write(cpu_read_reg(RT_HL), val);
    } else{
        cpu_set_reg(ctx->cur_inst->reg_1, val);
        val = cpu_read_reg(ctx->cur_inst->reg_1);
    }

    if ((ctx->cur_opcode & 0x0B) == 0x0B) {         // 0xB do not change flags
        return;
    }

    cpu_set_flags(ctx, val == 0, 1, (val & 0x0F) == 0x0F, -1);
}

// Sub instruction
static void proc_sub(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) - ctx->fetched_data;       // subtract reg value by literal value
    
    int z = val == 0;
    int h = ((int)cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) - ((int)ctx->fetched_data & 0xF) < 0;
    int c = ((int)cpu_read_reg(ctx->cur_inst->reg_1)) - ((int)ctx->fetched_data) < 0;

    cpu_set_reg(ctx->cur_inst->reg_1, val);
    cpu_set_flags(ctx, z, 1, h, c);
}

// Sbc instruction
static void proc_sbc(cpu_context *ctx) {
    u8 val = ctx->fetched_data + CPU_FLAG_C;        // subtracted value is literal plus carry bit

    int z = cpu_read_reg(ctx->cur_inst->reg_1) - val == 0;

    int h = ((int)cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) 
        - ((int)ctx->fetched_data & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)cpu_read_reg(ctx->cur_inst->reg_1)) 
        - ((int)ctx->fetched_data) - ((int)CPU_FLAG_C) < 0;

    cpu_set_reg(ctx->cur_inst->reg_1, cpu_read_reg(ctx->cur_inst->reg_1) - val);
    cpu_set_flags(ctx, z, 1, h, c);
}

// Adc instruction
static void proc_adc(cpu_context *ctx) {
    u16 u = ctx->fetched_data;
    u16 a = ctx->regs.a;
    u16 c = CPU_FLAG_C;

    ctx->regs.a = (a + u + c) & 0xFF;           // add literal and carry bit to the a reg

    cpu_set_flags(ctx, ctx->regs.a == 0, 0,
        (a & 0xF) + (u & 0xF) + c > 0xF,
        a + u + c > 0xFF);
}

// Add instruction
static void proc_add(cpu_context *ctx) {
    u32 val = cpu_read_reg(ctx->cur_inst->reg_1) + ctx->fetched_data;       // add reg value and literal

    bool is_16bit = is_16_bit(ctx->cur_inst->reg_1);

    if (is_16bit) {         // 16 bit add
        emu_cycles(1);
    }

    if (ctx->cur_inst->reg_1 == RT_SP) {            
        val = cpu_read_reg(ctx->cur_inst->reg_1) + (char)ctx->fetched_data;         // casting to char because literal may be negative
    }

    // 8 bit add
    int z = (val & 0xFF) == 0;
    int h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
    int c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;

    if (is_16bit) {         // 16 bit add
        z = -1;
        h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xFFF) + (ctx->fetched_data & 0xFFF) >= 0x1000;
        u32 n = ((u32)cpu_read_reg(ctx->cur_inst->reg_1)) + ((u32)ctx->fetched_data);
        c = n >= 0x10000;
    }

    if (ctx->cur_inst->reg_1 == RT_SP) {            // stack pointer treated as 8 bit add
        z = 0;
        h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
        c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) > 0x100;
    }

    cpu_set_reg(ctx->cur_inst->reg_1, val & 0xFFFF);
    cpu_set_flags(ctx, z, 0, h, c);
    
}

static IN_PROC processors[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_LD] = proc_ld,
    [IN_LDH] = proc_ldh,
    [IN_JP] = proc_jp,
    [IN_DI] = proc_di,
    [IN_POP] = proc_pop,
    [IN_PUSH] = proc_push,
    [IN_JR] = proc_jr,
    [IN_CALL] = proc_call,
    [IN_RET] = proc_ret,
    [IN_RETI] = proc_reti,
    [IN_RST] = proc_rst,
    [IN_DEC] = proc_dec,
    [IN_INC] = proc_inc,
    [IN_ADD] = proc_add,
    [IN_ADC] = proc_adc,
    [IN_SUB] = proc_sub,
    [IN_SBC] = proc_sbc,
    [IN_AND] = proc_and,
    [IN_XOR] = proc_xor,
    [IN_OR] = proc_or,
    [IN_CP] = proc_cp,
    [IN_CB] = proc_cb,
    [IN_RRCA] = proc_rrca,
    [IN_RLCA] = proc_rlca,
    [IN_RRA] = proc_rra,
    [IN_RLA] = proc_rla,
    [IN_STOP] = proc_stop,
    [IN_HALT] = proc_halt,
    [IN_DAA] = proc_daa,
    [IN_CPL] = proc_cpl,
    [IN_SCF] = proc_scf,
    [IN_CCF] = proc_ccf,
    [IN_EI] = proc_ei
};

// Run processor for each instruction type
IN_PROC inst_get_processor(in_type type) {
    return processors[type];
}
