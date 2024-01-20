#include <ppu.h>
#include <lcd.h>

// Push pixel value to the end of the pipeline
// Classic linked-list implementation: https://www.geeksforgeeks.org/data-structures/linked-list/
void pixel_fifo_push(u32 value) {
    fifo_entry *next = malloc(sizeof(fifo_entry));
    next->next = NULL;
    next->value = value;

    // Check if there are no entries in the pipeline
    if (!ppu_get_context()->pfc.pixel_fifo.head) {
        ppu_get_context()->pfc.pixel_fifo.head = ppu_get_context()->pfc.pixel_fifo.tail = next;
    } else {
        ppu_get_context()->pfc.pixel_fifo.tail->next = next;
        ppu_get_context()->pfc.pixel_fifo.tail = next;
    }

    ppu_get_context()->pfc.pixel_fifo.size++;
}

// Pop the first element out the pipeline
u32 pixel_fifo_pop() {
    // Check if pipeline is empty
    if (ppu_get_context()->pfc.pixel_fifo.size <= 0) {
        fprintf(stderr, "ERROR IN PIXEL FIFO\n");
        exit(-8);
    }

    fifo_entry *popped = ppu_get_context()->pfc.pixel_fifo.head;
    ppu_get_context()->pfc.pixel_fifo.head = popped->next;
    ppu_get_context()->pfc.pixel_fifo.size--;

    u32 val = popped->value;
    free(popped);

    return val;
}

bool pipeline_fifo_add() {
    // Check if pixel FIFO is full
    if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
        return false;
    }

    int x = ppu_get_context()->pfc.fetch_x - (8 - (lcd_get_context()->scroll_x % 8));
    for (int i = 0; i < 8; i++) {
        int bit = 7 - i;
        u8 hi = !!(ppu_get_context()->pfc.bgw_fetch_data[1] & (1 << bit));
        u8 lo = !!(ppu_get_context()->pfc.bgw_fetch_data[2] & (1 << bit)) << 1;
        u32 color = lcd_get_context()->bg_colors[hi | lo];

        if (x >= 0) {
            pixel_fifo_push(color);
            ppu_get_context()->pfc.fifo_x++;
        }
    }

    return true;
}

void pipeline_fetch() {
    switch (ppu_get_context()->pfc.cur_fetch_state) {
        case FS_TILE: {
            if (LCDC_BGW_ENABLE) {
                ppu_get_context()->pfc.bgw_fetch_data[0] = bus_read(LCDC_BG_MAP_AREA + 
                    (ppu_get_context()->pfc.map_x / 8) + 
                    (((ppu_get_context()->pfc.map_y / 8)) * 32));
                
                if (LCDC_BGW_DATA_AREA == 0x8800) {
                    ppu_get_context()->pfc.bgw_fetch_data[0] += 128;  // increment tile id
                }
            }

            ppu_get_context()->pfc.cur_fetch_state = FS_DATA0;
            ppu_get_context()->pfc.fetch_x += 8;
        } break;

        case FS_DATA0: {
            ppu_get_context()->pfc.bgw_fetch_data[1] = bus_read(LCDC_BGW_DATA_AREA + 
                (ppu_get_context()->pfc.bgw_fetch_data[0] * 16) + 
                ppu_get_context()->pfc.tile_y);

            ppu_get_context()->pfc.cur_fetch_state = FS_DATA1;
        } break;

        case FS_DATA1: {
            ppu_get_context()->pfc.bgw_fetch_data[2] = bus_read(LCDC_BGW_DATA_AREA + 
                (ppu_get_context()->pfc.bgw_fetch_data[0] * 16) + 
                ppu_get_context()->pfc.tile_y + 1);

            ppu_get_context()->pfc.cur_fetch_state = FS_IDLE;
        } break;

        case FS_IDLE: {
            ppu_get_context()->pfc.cur_fetch_state = FS_PUSH;
        } break;

        case FS_PUSH: {
            if (pipeline_fifo_add()) {
                ppu_get_context()->pfc.cur_fetch_state = FS_TILE;
            }
        } break;
    }
}

void pipeline_push_pixel() {
    if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
        u32 pixel_data = pixel_fifo_pop();

        if (ppu_get_context()->pfc.line_x >= (lcd_get_context()->scroll_x % 8)) {
            ppu_get_context()->video_buffer[ppu_get_context()->pfc.pushed_x + 
            (lcd_get_context()->ly * XRES)] = pixel_data;

            ppu_get_context()->pfc.pushed_x++;
        }

        ppu_get_context()->pfc.line_x++;
    }
}

void pipeline_process() {
    // Calculate map X and Y value
    ppu_get_context()->pfc.map_x = (ppu_get_context()->pfc.fetch_x + lcd_get_context()->scroll_x);
    ppu_get_context()->pfc.map_y = (lcd_get_context()->ly + lcd_get_context()->scroll_y);
    ppu_get_context()->pfc.tile_y = ((lcd_get_context()->ly + lcd_get_context()->scroll_y) % 8) * 2;

    // Check if line is even-indexed
    if (!(ppu_get_context()->line_ticks & 1)) {
        pipeline_fetch();
    }

    pipeline_push_pixel();
}
