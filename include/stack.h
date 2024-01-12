#ifndef __STACK_H__
#define __STACK_H__

#include <common.h>

void stack_push(u8 data);
void stack_push16(u16 data);

u8 stack_pop();
u16 stack_pop16();

#endif /* __STACK_H__ */
