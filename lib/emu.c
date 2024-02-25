#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>
#include <dma.h>
#include <ppu.h>
#include <audio.h>

// TODO: add windows alternative
#include <pthread.h>
#include <unistd.h>

#include <SDL2/SDL.h>

/*

    Emulator components:

    |Cart|
    |CPU|
    |Address Bus|
    |Pixel Processing Unit (PPU)|
    |Timer|

*/

static emu_context ctx;

emu_context *emu_get_context() {
    return &ctx;
}

void *cpu_run(void *p) {
    // Initialize CPU and timer
    timer_init();
    cpu_init();
    ppu_init();

    // Setting initial context variables
    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    // Main game loop
    while (ctx.running) {
        if (ctx.paused) {
            delay(10);
            continue;
        }
        
        if (!cpu_step()) {
            printf("CPU Stopped\n");
            return 0;
        }
    }

    return 0;
}

double get_monitor_refresh_ms() {
  return 1000.0 / 60;
}

int emu_run(int argc, char **argv) {
    // Checks to see if a ROM file is passed in
    if (argc < 2) {
        printf("Usage: emu <rom_file>\n");
        return -1;
    }

    // Checks to see if the cartridge can be loaded
    if (!cart_load(argv[1])) {
        printf("Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }

    printf("Cart loaded..\n");

    ui_init();
    audio_init();

    // Declare main thread
    pthread_t t1;

    // Create main thread to run cpu_run
    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "FAILED TO START MAIN CPU THREAD!\n");
        return -1;
    }

    u32 prev_frame = 0;
    double refresh_ms = get_monitor_refresh_ms();

    // Constantly loops until die is true
    while (!ctx.die) {
        usleep(1000);
        ui_handle_events();
        u32 event = audio_run_ms(refresh_ms);

        // Only update UI when frame changes to save time
        if (prev_frame != ppu_get_context()->current_frame) {
            ui_update();
        }
        prev_frame = ppu_get_context()->current_frame;
    }

    return 0;
}

void emu_cycles(int cpu_cycles) {
     for (int i = 0; i < cpu_cycles; i++) {
        for (int n = 0; n < 4; n++) {
            ctx.ticks++;
            timer_tick();
            ppu_tick();
        }

        dma_tick();
    }
}
