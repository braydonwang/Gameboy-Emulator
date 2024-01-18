#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>

//TODO add windows alternative
#include <pthread.h>
#include <unistd.h>

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

    // Declare main thread
    pthread_t t1;

    // Create main thread to run cpu_run
    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "FAILED TO START MAIN CPU THREAD!\n");
        return -1;
    }

    // Constantly loops until die is true
    while (!ctx.die) {
        usleep(1000);
        ui_handle_events();

        ui_update();
    }

    return 0;
}

void emu_cycles(int cpu_cycles) {
    int n = cpu_cycles * 4;

    for (int i = 0; i < n; i++) {
        ctx.ticks++;
        timer_tick();
    }
}
