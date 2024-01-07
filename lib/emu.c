#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <SDL2/SDL.h>
#include </Library/Frameworks/SDL2_ttf.framework/Headers/SDL_ttf.h>

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


void delay(u32 ms) {
    SDL_Delay(ms);
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

    // Initializing graphics and fonts
    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL INIT\n");
    TTF_Init();
    printf("TTF INIT\n");

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
            return -3;
        }

        ctx.ticks++;
    }

    return 0;
}
