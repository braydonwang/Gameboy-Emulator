#ifndef __RAM_H__
#define __RAM_H__

#include <common.h>

u8 wram_read(u16 address);
void wram_write(u16 address, u8 value);

u8 hram_read(u16 address);
void hram_write(u16 address, u8 value);

#endif /* __RAM_H__ */
