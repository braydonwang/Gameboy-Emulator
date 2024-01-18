#ifndef __IO_H__
#define __IO_H__

#include <common.h>

u8 io_read(u16 address);
void io_write(u16 address, u8 value);

#endif /* __IO_H__ */
