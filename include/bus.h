#ifndef __BUS_H__
#define __BUS_H__

#include <common.h>

u8 bus_read(u16 address);
void bus_write(u16 address, u8 value);

u16 bus_read16(u16 address);
void bus_write16(u16 address, u16 value);

#endif /* __BUS_H__ */
