#include <stack.h>
#include <cpu.h>
#include <bus.h>

// Push onto stack by decrementing stack pointer and writing data to bus
void stack_push(u8 data) {
    cpu_get_regs()->sp--;
    bus_write(cpu_get_regs()->sp, data);
}

// Push 16-bit value onto stack
void stack_push16(u16 data) {
    stack_push((data >> 8) & 0xFF);
    stack_push(data & 0xFF);
}

// Read value at stack pointer, then increment
u8 stack_pop() {
    return bus_read(cpu_get_regs()->sp++);
}

// Read 16-bit value at stack pointer
u16 stack_pop16() {
    u16 lo = stack_pop();
    u16 hi = stack_pop();
    return (hi << 8) | lo;
}
