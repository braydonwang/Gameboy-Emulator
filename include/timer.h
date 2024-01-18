#ifndef __TIMER_H__
#define __TIMER_H__

#include <common.h>

// Timer context including divider registers
// Reference: https://gbdev.io/pandocs/Timer_and_Divider_Registers.html
typedef struct {
    u16 div;           // divider register (address FF04)
    u8 tima;           // timer counter    (address FF05)
    u8 tma;            // timer modulo     (address FF06)
    u8 tac;            // timer control    (address FF07)
} timer_context;

void timer_init();
void timer_tick();

void timer_write(u16 address, u8 value);
u8 timer_read(u16 address);

timer_context *timer_get_context();

#endif /* __TIMER_H__ */
