#ifndef __CART_H__
#define __CART_H__

#include <common.h>

// Information included in cartridge header referenced from this doc:
// https://gbdev.io/pandocs/The_Cartridge_Header.html

typedef struct {
    u8 entry[4];
    u8 logo[0x30];

    char title[16];
    u16 new_lic_code;
    u8 sgb_flag;
    u8 type;
    u8 rom_size;
    u8 ram_size;
    u8 dest_code;
    u8 lic_code;
    u8 version;
    u8 checksum;
    u16 global_checksum;
} rom_header;

bool cart_load(char *cart);

u8 cart_read(u16 address);
void cart_write(u16 address, u8 value);

#endif /* __CART_H__ */
