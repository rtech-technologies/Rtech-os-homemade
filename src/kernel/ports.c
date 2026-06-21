#include "ports.h"

/**
 * Read a byte from the specified port
 */
uint8_t port_byte_in(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

void port_byte_out(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %%al, %%dx" : : "a" (data), "d" (port));
}

uint16_t port_word_in(uint16_t port) {
    uint16_t result;
    __asm__ volatile("inw %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

void port_word_out(uint16_t port, uint16_t data) {
    __asm__ volatile("outw %%ax, %%dx" : : "a" (data), "d" (port));
}
