#include <cart.h>
#include <rom_types.h>
#include <lic_codes.h>

typedef struct {
    char filename[1024];
    u32 rom_size;
    u8 *rom_data;
    rom_header *header;
} cart_context;

static cart_context ctx;

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

    // Displaying cartridge context
    printf("Cartridge Loaded:\n");
    printf("\t Title    : %s\n", ctx.header->title);
    printf("\t Type     : %2.2X (%s)\n", ctx.header->type, cart_type_name());
    printf("\t ROM Size : %d KB\n", 32 << ctx.header->rom_size);
    printf("\t RAM Size : %2.2X\n", ctx.header->ram_size);
    printf("\t LIC Code : %2.2X (%s)\n", ctx.header->lic_code, cart_lic_name());
    printf("\t ROM Vers : %2.2X\n", ctx.header->version);

    // Run header checksum to validate ROM file (https://gbdev.io/pandocs/The_Cartridge_Header.html#014d--header-checksum)
    u16 checksum = 0;
    for (u16 address = 0x0134; address <= 0x014C; address++) {
        checksum = checksum - ctx.rom_data[address] - 1;
    }

    printf("\t Checksum : %2.2X (%s)\n", ctx.header->checksum, (checksum & 0xFF) ? "PASSED" : "FAILED");

    return true;
}

u8 cart_read(u16 address) {
    // ROM only type support, for now...
    return ctx.rom_data[address];
}

void cart_write(u16 address, u8 value) {
    // ROM only type support, for now...
    NO_IMPL
}

