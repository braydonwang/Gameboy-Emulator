#include <ppu.h>
#include <lcd.h>
#include <cpu.h>
#include <interrupts.h>
#include <string.h>
#include <cart.h>

void pipeline_fifo_reset();
void pipeline_process();
bool window_visible();

void increment_ly() {
    // Increment window line if window is visible
    if (window_visible() && lcd_get_context()->ly >= lcd_get_context()->win_y &&
        lcd_get_context()->ly < lcd_get_context()->win_y + YRES) {
            ppu_get_context()->window_line++;
        }

    // Increment ly
    lcd_get_context()->ly++;

    // Special case where ly == ly_compare
    if (lcd_get_context()->ly == lcd_get_context()->ly_compare) {
        LCDS_LYC_SET(1);

        if (LCDS_STAT_INT(SS_LYC)) {
            cpu_request_interrupt(IT_LCD_STAT);
        }
    } else {
        LCDS_LYC_SET(0);
    }
}

// load all sprites on a given line
void load_line_sprites() {
    int cur_y = lcd_get_context()->ly;

    u8 sprite_height = LCDC_OBJ_HEIGHT;
    memset(ppu_get_context()->line_entry_array, 0, sizeof(ppu_get_context()->line_entry_array));

    for (int i = 0; i < 40; i++) { // each of 40 oam entries
        oam_entry e = ppu_get_context()->oam_ram[i];

        if (!e.x) {
            // x = 0 means not visible
            continue;
        }

        if (ppu_get_context()->line_sprite_count >= 10) {
            // max 10 sprites per line
            break;
        }

        if (e.y <= cur_y + 16 && e.y + sprite_height > cur_y + 16) {
            // this sprite is on the current line
            oam_line_entry *entry = &ppu_get_context()->line_entry_array[ppu_get_context()->line_sprite_count++];

            entry->entry = e;
            entry->next = NULL;

            if (!ppu_get_context()->line_sprites || ppu_get_context()->line_sprites->entry.x > e.x) {
                entry->next = ppu_get_context()->line_sprites;
                ppu_get_context()->line_sprites = entry;
                continue;
            }

            // sorting, otherwise some sprites will not render properly
            oam_line_entry *le = ppu_get_context()->line_sprites;
            oam_line_entry *prev = le;

            while (le) {
                if (le->entry.x > e.x) {
                    prev->next = entry; // put at end of the linked list
                    entry->next = le;
                    break;
                }

                if (!le->next) {
                    le->next = entry;
                    break;
                }

                prev = le;
                le = le->next;
            }
        }
    }
}

void ppu_mode_oam() {
    // Change PPU mode
    if (ppu_get_context()->line_ticks >= 80) {
        LCDS_MODE_SET(MODE_XFER);

        // Initialize pixel FIFO context
        ppu_get_context()->pfc.cur_fetch_state = FS_TILE;
        ppu_get_context()->pfc.line_x = 0;
        ppu_get_context()->pfc.fetch_x = 0;
        ppu_get_context()->pfc.pushed_x = 0;
        ppu_get_context()->pfc.fifo_x = 0;
    }

    if (ppu_get_context()->line_ticks == 1) {
        // read oam on the first tick only...
        ppu_get_context()->line_sprites = 0;
        ppu_get_context()->line_sprite_count = 0;

        load_line_sprites();
    }
}

void ppu_mode_xfer() {
    pipeline_process();

    // Change PPU mode if pushed pixels exceeds X resolution
    if (ppu_get_context()->pfc.pushed_x >= XRES) {
        pipeline_fifo_reset();
        LCDS_MODE_SET(MODE_HBLANK);

        // Check if STAT interrupt is set
        if (LCDS_STAT_INT(SS_HBLANK)) {
            cpu_request_interrupt(IT_LCD_STAT);
        }
    }
}

void ppu_mode_vblank() {
    // Move to next line
    if (ppu_get_context()->line_ticks >= TICKS_PER_LINE) {
        increment_ly();

        // Change ppu mode
        if (lcd_get_context()->ly >= LINES_PER_FRAME) {
            LCDS_MODE_SET(MODE_OAM);
            // Reset ly
            lcd_get_context()->ly = 0;
            ppu_get_context()->window_line = 0;
        }

        // Reset ticks
        ppu_get_context()->line_ticks = 0;
    }
}

// Frame variables
static u32 target_frame_time = 1000 / 60;
static long prev_frame_time = 0;
static long start_timer = 0;
static long frame_count = 0;

void ppu_mode_hblank() {
    // Move to next line
    if (ppu_get_context()->line_ticks >= TICKS_PER_LINE) {
        increment_ly();

        // Change mode
        if (lcd_get_context()->ly >= YRES) {
            LCDS_MODE_SET(MODE_VBLANK);

            // VBlank interrupt
            cpu_request_interrupt(IT_VBLANK);

            if (LCDS_STAT_INT(SS_VBLANK)) {
                // LCD stat interrupt
                cpu_request_interrupt(IT_LCD_STAT);
            }

            // Increment frame
            ppu_get_context()->current_frame++;

            // Calculate FPS
            u32 end = get_ticks();
            u32 frame_time = end - prev_frame_time;

            // Limit FPS
            if (frame_time < target_frame_time) {
                delay((target_frame_time - frame_time));
            }

            if (end - start_timer >= 1000) {
                u32 fps = frame_count;
                start_timer = end;
                frame_count = 0;

                printf("FPS: %d\n", fps);

                if (cart_need_save()) {
                    cart_battery_save();
                }
            }

            frame_count++;
            prev_frame_time = get_ticks();


        } else {
            LCDS_MODE_SET(MODE_OAM);
        }
        
        // Reset ticks
        ppu_get_context()->line_ticks = 0;
    }
}
