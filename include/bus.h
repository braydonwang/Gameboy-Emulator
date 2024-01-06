#ifndef __BUS_H__
#define __BUS_H__

#include <common.h>

u8 bus_read(u16 address);
void bus_write(u16 address, u8 value);

#endif /* __BUS_H__ */
