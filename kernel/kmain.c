#include "include/asm.h"
#include "include/mon.h"

void kmain() {
    mon_init(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    puts("POIESIS system v0.0.1\nCopyright (C) 2023 Bruno Kushnir");
    
    for (;;) {
        cli();
        hlt();
    }
}
