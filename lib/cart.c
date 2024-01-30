#include <cart.h>
#include <rom_types.h>
#include <lic_codes.h>
#include <string.h>

typedef struct {
    char filename[1024];
    u32 rom_size;
    u8 *rom_data;
    rom_header *header;

    // MBC1 Data
    bool ram_enabled;
    bool ram_banking;

    u8 *rom_bank_x;
    u8 banking_mode;

    u8 rom_bank_value;
    u8 ram_bank_value;

    u8 *ram_bank;               // currently selected RAM bank
    u8 *ram_banks[16];          // all RAM banks (max 16 total)

    // Battery Data
    bool battery;               // whether it has battery or not
    bool need_save;             // whether we should save battery backup or not
} cart_context;

static cart_context ctx;

bool cart_need_save() {
    return ctx.need_save;
}

// MBC1 mapper found between cartridge codes 1 and 3 in cartridge header
// Reference: https://gbdev.io/pandocs/The_Cartridge_Header.html#0147--cartridge-type 
bool cart_mbc1() {
    return BETWEEN(ctx.header->type, 1, 3);
}

// Returns true if cartridge type is MBC1+RAM+BATTERY
bool cart_battery() {
    return ctx.header->type == 3;
}

// Returns licensee code string based on cartridge header
const char *cart_lic_name() {
    if (ctx.header->new_lic_code <= 0xA4) {
        return LIC_CODE[ctx.header->lic_code];
    }
    return "UNKNOWN";
}

// Returns cartridge type string based on cartridge header
const char *cart_type_name() {
    if (ctx.header->type <= 0x22) {
        return ROM_TYPES[ctx.header->type];
    }
    return "UNKNOWN";
}

void cart_setup_banking() {
    for (int i = 0; i < 16; i++) {
        ctx.ram_banks[i] = 0;

        // Check if number of banks corresponds to RAM size
        if ((ctx.header->ram_size == 2 && i == 0) || (ctx.header->ram_size == 3 && i < 4) ||
           (ctx.header->ram_size == 4 && i < 16) || (ctx.header->ram_size == 5 && i < 8)) {
            ctx.ram_banks[i] = malloc(0x2000);
            memset(ctx.ram_banks[i], 0, 0x2000);
        }
    }

    ctx.ram_bank = ctx.ram_banks[0];
    ctx.rom_bank_x = ctx.rom_data + 0x4000;   // RROM bank 1
}

// Load each entry from cartridge header, returns true on success
// Reference: https://gbdev.io/pandocs/The_Cartridge_Header
bool cart_load(char *cart) {
    snprintf(ctx.filename, sizeof(ctx.filename), "%s", cart);

    FILE *fp = fopen(cart, "r");

    // Check if file with specified filename exists
    if (!fp) {
        printf("Failed to open: %s\n", cart);
        return false;
    }

    printf("Opened: %s\n", ctx.filename);

    // Seek to end of file to determine size of it
    fseek(fp, 0, SEEK_END);
    ctx.rom_size = ftell(fp);

    // Move file descriptor back to the beginning of the file
    rewind(fp);

    // Allocate and read in entire file data 
    ctx.rom_data = malloc(ctx.rom_size);
    fread(ctx.rom_data, ctx.rom_size, 1, fp);
    fclose(fp);

    // Read in header located at 0x100 (https://gbdev.io/pandocs/The_Cartridge_Header.html#0100-0103--entry-point)
    ctx.header = (rom_header *)(ctx.rom_data + 0x100);
    ctx.header->title[15] = 0;
    ctx.battery = cart_battery();
    ctx.need_save = false;

    // Displaying cartridge context
    printf("Cartridge Loaded:\n");
    printf("\t Title    : %s\n", ctx.header->title);
    printf("\t Type     : %2.2X (%s)\n", ctx.header->type, cart_type_name());
    printf("\t ROM Size : %d KB\n", 32 << ctx.header->rom_size);
    printf("\t RAM Size : %2.2X\n", ctx.header->ram_size);
    printf("\t LIC Code : %2.2X (%s)\n", ctx.header->lic_code, cart_lic_name());
    printf("\t ROM Vers : %2.2X\n", ctx.header->version);

    cart_setup_banking();

    // Run header checksum to validate ROM file (https://gbdev.io/pandocs/The_Cartridge_Header.html#014d--header-checksum)
    u16 checksum = 0;
    for (u16 address = 0x0134; address <= 0x014C; address++) {
        checksum = checksum - ctx.rom_data[address] - 1;
    }

    printf("\t Checksum : %2.2X (%s)\n", ctx.header->checksum, (checksum & 0xFF) ? "PASSED" : "FAILED");

    if (ctx.battery) {
        cart_battery_load();
    }

    return true;
}

void cart_battery_load() {
    char fn[1048];
    sprintf(fn, "%s.battery", ctx.filename);
    FILE *fp = fopen(fn, "rb");

    if (!fp) {
        fprintf(stderr, "FAILED TO OPEN: %s\n", fn);
        return;
    }

    fread(ctx.ram_bank, 0x2000, 1, fp);
    fclose(fp);
}

void cart_battery_save() {
    char fn[1048];
    sprintf(fn, "%s.battery", ctx.filename);
    FILE *fp = fopen(fn, "wb");

    if (!fp) {
        fprintf(stderr, "FAILED TO OPEN: %s\n", fn);
        return;
    }

    fwrite(ctx.ram_bank, 0x2000, 1, fp);
    fclose(fp);
}

u8 cart_read(u16 address) {
    if (!cart_mbc1() || address < 0x4000) {
        return ctx.rom_data[address];
    }

    // Check if address is in the range A000-BFFF (RAM bank)
    // Reference: https://gbdev.io/pandocs/MBC1.html#a000bfff--ram-bank-0003-if-any
    if ((address & 0xE000) == 0xA000) {
        if (!ctx.ram_enabled || !ctx.ram_bank) {
            return 0xFF;
        }

        return ctx.ram_bank[address - 0xA000];
    }

    return ctx.rom_bank_x[address - 0x4000];
}

void cart_write(u16 address, u8 value) {
    if (!cart_mbc1()) {
        return;
    }

    // Check if address is in the range 0000-1FFF (RAM enable)
    // Reference: https://gbdev.io/pandocs/MBC1.html#00001fff--ram-enable-write-only
    if (address < 0x2000) {
        ctx.ram_enabled = ((value & 0xF) == 0xA);    // any value with 0xA in lower 4 bits enables RAM
    }

    // Check if address is in the range 2000-3FFF (ROM bank number)
    // Reference: https://gbdev.io/pandocs/MBC1.html#20003fff--rom-bank-number-write-only
    if ((address & 0xE000) == 0x2000) {
        if (value == 0) {
            value = 1;                              // if register set to 0, it behaves as if set to 1
        }

        value &= 0b11111;
        ctx.rom_bank_value = value;
        ctx.rom_bank_x = ctx.rom_data + (0x4000 * ctx.rom_bank_value);
    }

    // Check if address is in the range 4000-5FFF (RAM bank number)
    // Reference: https://gbdev.io/pandocs/MBC1.html#40005fff--ram-bank-number--or--upper-bits-of-rom-bank-number-write-only
    if ((address & 0xE000) == 0x4000) {
        ctx.ram_bank_value = value & 0b11;

        // Check if RAM banking is enabled
        if (ctx.ram_banking) {
            if (cart_need_save()) {
                cart_battery_save();            // save battery when switching RAM banks
            }
            ctx.ram_bank = ctx.ram_banks[ctx.ram_bank_value];
        }
    }

    // Check if address is in the range 6000-7FFF (Banking Mode Select)
    // Reference: https://gbdev.io/pandocs/MBC1.html#60007fff--banking-mode-select-write-only
    if ((address & 0xE000) == 0x6000) {
        ctx.banking_mode = value & 1;
        ctx.ram_banking = ctx.banking_mode;

        // Check if RAM banking is enabled
        if (ctx.ram_banking) {
            if (cart_need_save()) {
                cart_battery_save();            // save battery when switching RAM banks
            }
            ctx.ram_bank = ctx.ram_banks[ctx.ram_bank_value];
        }
    }

    // Check if address is in the range A000-BFFF (RAM bank)
    // Reference: https://gbdev.io/pandocs/MBC1.html#a000bfff--ram-bank-0003-if-any
    if ((address & 0xE000) == 0xA000) {
        if (!ctx.ram_enabled || !ctx.ram_bank) {
            return;
        }

        ctx.ram_bank[address - 0xA000] = value;

        if (ctx.battery) {
            ctx.need_save = true;
        }
    }
}

