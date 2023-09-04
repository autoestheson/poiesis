#include <stdint.h>
#include <stdbool.h>

#include "include/asm.h"
#include "include/fs.h"
#include "include/mon.h"

void panic(char *s) {
    puts(s);
    
    for (;;) {
        cli();
        hlt();
    }
}

bool match(char *p, char *q) {
    while (*p && *p == *q)
        p++, q++;
    return *p == *q;
}

struct {
    uint32_t disk_sz;
    uint32_t root_sz;
    uint32_t skip_sz;
} disk;

void disk_init() {
    putn(10, disk.disk_sz = *((uint32_t *)(0x7C00 + FS_DISK_BLOCKS_AT))); puts(" total, ");
    putn(10, disk.root_sz = *((uint32_t *)(0x7C00 + FS_ROOT_BLOCKS_AT))); puts(" root, ");
    putn(10, disk.skip_sz = *((uint32_t *)(0x7C00 + FS_SKIP_BLOCKS_AT))); puts(" reserved block(s)\n");
}

void disk_wait() {
    while ((inb(0x1F7) & 0xC0) != 0x40);
}

void load_block(void *dest, uint32_t block) {
    disk_wait();
    
    outb(0x1F2, 1);
    outb(0x1F3, block);
    outb(0x1F4, block >> 8);
    outb(0x1F5, block >> 16);
    outb(0x1F6, (block >> 24) | 0xE0);
    outb(0x1F7, 0x20);
    
    disk_wait();
    
    insl(0x1F0, dest, 512 / 4);
}

struct dirent stat(char *name) {
    struct dirent *buf = (void *)0x7C00;
    
    for (uint32_t block = 0;; block++) {
        load_block(buf, disk.skip_sz + disk.disk_sz / 128 + block);
        for (uint32_t entry = 0; entry < 512 / 64; entry++) {
            if (!buf[entry].parent_id || block >= disk.root_sz)
                panic("Argh!\nWe took yer booty.\n - Software Pirates");
            if (buf[entry].parent_id == FS_DIRENT_ROOT && buf[entry].type == FS_FTYPE_F && match(buf[entry].name, name))
                return buf[entry];
        }
    }
}

uint32_t load(void *dest, char *name) {
    uint32_t *buf = (void *)0x7C00;
    uint32_t id = stat(name).target_id;
    
    puts("Loading \""), puts(name), puts("\"");
    
    while (id != FS_TBLENT_NULL) {
        load_block(dest, id);
        dest += 512;
        load_block(buf, disk.skip_sz + id / 128);
        id = buf[id % 128];
        putc('.');
    }
    putc('\n');
}

void mem_init() {
    putn(10, *((uint32_t *)(0x604)) >> 10), puts(" KiB low, ");
    putn(10, *((uint32_t *)(0x608)) >> 10), putc('(');
    putn(10, *((uint32_t *)(0x60C)) >> 10), putc('+');
    putn(10, *((uint32_t *)(0x610)) >> 10), puts(") KiB high memory\n");
}

void boot2() {
    void (*entry)(void) = (void (*)(void))0x100000;
    
    mon_init(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    puts("POIESIS boot v0.1.0\nCopyright (C) 2023 Bruno Kushnir\n");
    
    putc('\n');
    
    mem_init();
    disk_init();
    
    putc('\n');
    
    load(entry, "kernel");
    
    putc('\n');
    
    entry();
    
    panic("But nobody came.\n");
}
