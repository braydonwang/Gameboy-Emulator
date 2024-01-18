#include <ppu.h>

static ppu_context ctx;

void ppu_init() {

}

void ppu_tick() {
    
}

void ppu_oam_write(u16 address, u8 value) {
    // when adressing buffer, use actual offset
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    u8 *p = (u8 *)ctx.oam_ram; // convert to byte array
    p[address] = value; // set value of byte array at that address
}

u8 ppu_oam_read(u16 address) {
    // when adressing buffer, use actual offset
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    u8 *p = (u8 *)ctx.oam_ram; // convert to byte array
    return p[address];
}

void ppu_vram_write(u16 address, u8 value) {
    ctx.vram[address - 0x8000] = value;
}
u8 ppu_vram_read(u16 address) {
    return ctx.vram[address - 0x8000];
}