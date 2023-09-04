#ifndef ASM_H
#define ASM_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port) : "memory");
    return val;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    asm volatile("inw %1, %0" : "=a"(val) : "Nd"(port) : "memory");
    return val;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    asm volatile("inl %1, %0" : "=a"(val) : "Nd"(port) : "memory");
    return val;
}

static inline void outsl(uint32_t port, void *addr, uint32_t size) {
    asm volatile("cld; rep outsl" : "=S"(addr), "=c"(size) : "d"(port), "0"(addr), "1"(size) : "cc");
}

static inline void insl(uint32_t port, void *addr, uint32_t size) {
    asm volatile("cld; rep insl" : "=D"(addr), "=c"(size) : "d"(port), "0"(addr), "1"(size) : "memory", "cc");
}

static inline void cli() {
    asm volatile("cli");
}

static inline void hlt() {
    asm volatile("hlt");
}

#endif