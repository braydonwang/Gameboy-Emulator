#ifndef __PPU_H__
#define __PPU_H__

#include <common.h>

static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE = 456;
static const int YRES = 144;
static const int XRES = 160;

// 5 different states of pixel fetcher
// Reference: https://gbdev.io/pandocs/pixel_fifo.html#fifo-pixel-fetcher
typedef enum {
    FS_TILE,                        // get tile
    FS_DATA0,                       // get data low
    FS_DATA1,                       // get data high
    FS_IDLE,                        // sleep/idle
    FS_PUSH                         // push pixels
} fetch_state;

// Single entry of FIFO pipeline
typedef struct _fifo_entry {
    struct _fifo_entry *next;       // pointer to next entry
    u32 value;                      // 32 bit color value
} fifo_entry;

// FIFO pipeline structure resembling a linked-list
typedef struct {
    fifo_entry *head;               // start of pipeline
    fifo_entry *tail;               // end of pipeline
    u32 size;
} fifo;

typedef struct {
    fetch_state cur_fetch_state;    // current pixel fetch state
    fifo pixel_fifo;                // fifo pipeline structure
    u8 line_x;                      // x coordinate of current scanline
    u8 pushed_x;                    // x coordinate of pushed pixel
    u8 fetch_x;                     // x coordinate of fetched pixel
    u8 bgw_fetch_data[3];           // fetched background pixels
    u8 fetch_entry_data[6];         // OAM data
    u8 map_x;                       // x coordinate of tilemap
    u8 map_y;                       // y coordinate of tilemap
    u8 tile_y;                      // y coordinate of object tile
    u8 fifo_x;                      // x coordinate of fifo pipeline
} pixel_fifo_context;

typedef struct {
    u8 y;
    u8 x;
    u8 tile;

    unsigned f_cgb_pn : 3;          // coloured game boy palette number, 3 bits
    unsigned f_cgb_vram_bank : 1;   // tile vram number
    unsigned f_pn : 1;              // palette number
    unsigned f_x : 1;               // should sprite be flipped
    unsigned f_y : 1;               // should sprite be flipped
    unsigned f_bgp : 1;             // background priority

} oam_entry;

/*
    Bit7   BG and Window over OBJ (0=No, 1=BG and Window colors 1-3 over the OBJ)
    Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
    Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
    Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
    Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
    Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
 */

typedef struct _oam_line_entry {
    oam_entry entry;
    struct _oam_line_entry *next;
} oam_line_entry;

typedef struct {
    oam_entry oam_ram[40];
    u8 vram[0x2000];                // video ram

    pixel_fifo_context pfc;

    u8 line_sprite_count; // 0 to 10 sprites
    oam_line_entry *line_sprites; // linked list of current sprites on line
    oam_line_entry line_entry_array[10]; // memory to use for list, so we don't need to call malloc and free repeatedly

    u8 fetched_entry_count; // for the fifo fetching process
    oam_entry fetched_entries[3]; // entries fetched during pipeline, fetch 3 entries per section of pixels in the fifo

    u32 current_frame;
    u32 line_ticks;
    u32 *video_buffer;
} ppu_context;

void ppu_init();
void ppu_tick();

void ppu_oam_write(u16 address, u8 value);
u8 ppu_oam_read(u16 address);

void ppu_vram_write(u16 address, u8 value);
u8 ppu_vram_read(u16 address);

ppu_context *ppu_get_context();

void pipeline_fifo_reset();
void pipeline_process();

#endif /* __PPU_H__ */
