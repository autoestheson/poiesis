#include <stdint.h>

#include "include/asm.h"
#include "include/mon.h"

static struct {
    uint16_t *buf;
    uint16_t attr;
    uint8_t row;
    uint8_t col;
} mon;

static void update_cursor() {
    uint16_t cursor = mon.row * 80 + mon.col;
    outb(0x3D4, 14);
    outb(0x3D5, cursor >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, cursor);
}

static void scroll() {
    uint16_t space = mon.attr | ' ';
    
    if (mon.row >= 25) {
        for (int i = 0; i < 80 * 24; i++)
            mon.buf[i] = mon.buf[i + 80];
        for (int i = 80 * 24; i < 80 * 25; i++)
            mon.buf[i] = space;
        mon.row = 24;
    }
}

void mon_init(enum vga_color fg, enum vga_color bg) {
    mon.buf = (uint16_t *)0xB8000;
    mon.attr = bg << 12 | fg << 8;
    clear(' ');
}

void clear(char fill) {
    uint16_t space = mon.attr | fill;
    
    for (int i = 0; i < 80 * 25; i++)
        mon.buf[i] = space;
    
    mon.row = mon.col = 0;
    update_cursor();
}

void putc(char c) {
    switch (c) {
    case '\b':
        mon.col--;
        mon.buf[mon.row * 80 + mon.col] = mon.attr | c;
        break;
    case '\t':
        mon.col = (mon.col + 8) & ~7;
        break;
    case '\n':
        mon.row++;
    case '\r':
        mon.col = 0;
        break;
    default:
        mon.buf[mon.row * 80 + mon.col] = mon.attr | c;
        mon.col++;
    }
    
    scroll();
    update_cursor();
}

void puts(char *s) {
    while (*s)
        putc(*s++);
}

void putn(uint8_t r, uint32_t n) {
    char s[33], *p, *q, c;
    uint32_t m;
    
    if (r < 2 || r > 36)
        return;
    
    p = q = s;
    
    do {
        m = n;
        n /= r;
        *p++ = "ZYXWVUTSRQPONMLKJIHGFEDCBA9876543210123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[35 + (m - n * r)];
    } while (n);
    
    if (m < 0)
        *p++ = '-';
    
    *p-- = '\0';
    
    while (q < p) {
        c = *p;
        *p-- = *q;
        *q++ = c;
    }
    
    puts(s);
}
