#include <bus.h>
#include <cart.h>
#include <ram.h>
#include <cpu.h>
#include <io.h>
#include <ppu.h>
#include <dma.h>

/*
    Memory Map Addresses
    Reference: https://gbdev.io/pandocs/Memory_Map.html#memory-map

    0x0000 - 0x3FFF : ROM Bank 0
    0x4000 - 0x7FFF : ROM Bank 1 - Switchable
    0x8000 - 0x97FF : CHR RAM
    0x9800 - 0x9BFF : BG Map 1
    0x9C00 - 0x9FFF : BG Map 2
    0xA000 - 0xBFFF : Cartridge RAM
    0xC000 - 0xCFFF : RAM Bank 0
    0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
    0xE000 - 0xFDFF : Reserved - Echo RAM
    0xFE00 - 0xFE9F : Object Attribute Memory
    0xFEA0 - 0xFEFF : Reserved - Unusable
    0xFF00 - 0xFF7F : I/O Registers
    0xFF80 - 0xFFFE : Zero Page
*/

u8 bus_read(u16 address) {
    if (address < 0x8000) {
        // Reading ROM data
        return cart_read(address);
    } else if (address < 0xA000) {
        // Character and Map data
        return ppu_vram_read(address);
    } else if (address < 0xC000) {
        // Cartridge RAM
        return cart_read(address);
    } else if (address < 0xE000) {
        // Working RAM (WRAM)
        return wram_read(address);
    } else if (address < 0xFE00) {
        // Reserved Echo RAM (unusable)
        return 0;
    } else if (address < 0xFEA0) {
        // Object Attribute Memory (OAM)
        if (dma_transferring()) {
            return 0xFF;
        }
        return ppu_oam_read(address);
    } else if (address < 0xFF00) {
        // Reversed unusable section
        return 0;
    } else if (address < 0xFF80) {
        // I/O Registers
        return io_read(address);
    } else if (address == 0xFFFF) {
        // Interrupt Enable Register (IE)
        return cpu_get_ie_register();
    }

    // Zero Page or High RAM (HRAM)
    return hram_read(address);
}

void bus_write(u16 address, u8 value) {
    if (address < 0x8000) {
        // Writing ROM data
        cart_write(address, value);
        return;
    } else if (address < 0xA000) {
        // Character and Map data
        ppu_vram_write(address, value);
    } else if (address < 0xC000) {
        // Cartridge RAM
        cart_write(address, value);
    } else if (address < 0xE000) {
        // Working RAM (WRAM)
        wram_write(address, value);
    } else if (address < 0xFE00) {
        // Reserved Echo RAM (unusable)
    } else if (address < 0xFEA0) {
        // Object Attribute Memory (OAM)
        if (dma_transferring()) {
            return;
        }
        ppu_oam_write(address, value);
    } else if (address < 0xFF00) {
        // Reversed unusable section
    } else if (address < 0xFF80) {
        // I/O Registers
        io_write(address, value);
    } else if (address == 0xFFFF) {
        // Interrupt Enable Register (IE)
        cpu_set_ie_register(value);
    } else {
        // Zero Page or High RAM (HRAM)
        hram_write(address, value);
    }
}

u16 bus_read16(u16 address) {
    u16 lo = bus_read(address);
    u16 hi = bus_read(address + 1);

    return lo | (hi << 8);
}

void bus_write16(u16 address, u16 value) {
    bus_write(address + 1, (value >> 8) & 0xFF);
    bus_write(address, value & 0xFF);
}
