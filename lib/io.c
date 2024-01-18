#include <io.h>

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

    printf("UNSUPPORTED bus_write(%04X)\n", address);
}
