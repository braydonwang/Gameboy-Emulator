#include <io.h>
#include <timer.h>
#include <cpu.h>

/*
    Handling Serial Data Transfer (I/O)
    Reference: https://gbdev.io/pandocs/Serial_Data_Transfer_(Link_Cable).html
*/

static char serial_data[2];

u8 io_read(u16 address) {
    if (address == 0xFF01) {
        return serial_data[0];
    }

    if (address == 0xFF02) {
        return serial_data[1];
    }

    if (BETWEEN(address, 0xFF04, 0xFF07)) {
        return timer_read(address);
    }

    // Interrupt flag: https://gbdev.io/pandocs/Interrupts.html#ff0f--if-interrupt-flag
    if (address == 0xFF0F) {
        return cpu_get_int_flags();
    }

    printf("UNSUPPORTED bus_read(%04X)\n", address);
    return 0;
}

void io_write(u16 address, u8 value) {
    if (address == 0xFF01) {
        serial_data[0] = value;
        return;
    }

    if (address == 0xFF02) {
        serial_data[1] = value;
        return;
    }

    if (BETWEEN(address, 0xFF04, 0xFF07)) {
        timer_write(address, value);
        return;
    }

    // Interrupt flag: https://gbdev.io/pandocs/Interrupts.html#ff0f--if-interrupt-flag
    if (address == 0xFF0F) {
        cpu_set_int_flags(value);
    }

    printf("UNSUPPORTED bus_write(%04X)\n", address);
}
