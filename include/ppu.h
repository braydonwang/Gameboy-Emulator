#ifndef __PPU_H__
#define __PPU_H__

#include <common.h>

typedef struct {
    u8 y;
    u8 x;
    u8 tile;

    unsigned f_cgb_pn : 3; // coloured game boy palette number, 3 bits
    unsigned f_cgb_vram_bank : 1; // tile vram number
    unsigned f_pn : 1; // palette number
    unsigned f_x : 1; // should sprite be flipped
    unsigned f_y : 1; // should sprite be flipped
    unsigned f_bgp : 1; // background priority

} oam_entry;

/*
 Bit7   BG and Window over OBJ (0=No, 1=BG and Window colors 1-3 over the OBJ)
 Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
 Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
 Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
 Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
 Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
 */

typedef struct {
    oam_entry oam_ram[40];
    u8 vram[0x2000]; // video ram
} ppu_context;

void ppu_init();
void ppu_tick();

void ppu_oam_write(u16 address, u8 value);
u8 ppu_oam_read(u16 address);

void ppu_vram_write(u16 address, u8 value);
u8 ppu_vram_read(u16 address);

#endif /* __PPU_H__ */
