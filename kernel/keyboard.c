#include "keyboard.h"

// Inline IO functions for x86_64
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

#define PS2_DATA_PORT 0x60
#define PS2_CMD_PORT  0x64

static bool key_down[256];
static bool key_pressed[256];
static bool key_down_prev[256];

void init_keyboard(void) {
    for (int i = 0; i < 256; i++) {
        key_down[i] = false;
        key_pressed[i] = false;
        key_down_prev[i] = false;
    }
}

void update_keyboard(void) {
    // Save previous state for edge detection
    for (int i = 0; i < 256; i++) {
        key_down_prev[i] = key_down[i];
    }

    // Drain the PS/2 buffer (very basic polling, not interrupt driven yet)
    // The status register bit 0 is set if there is data
    while (inb(PS2_CMD_PORT) & 0x01) {
        uint8_t scancode = inb(PS2_DATA_PORT);

        bool release = (scancode & 0x80) != 0;
        scancode &= 0x7F;

        if (release) {
            key_down[scancode] = false;
        } else {
            key_down[scancode] = true;
        }
    }

    // Update pressed (edge triggered)
    for (int i = 0; i < 256; i++) {
        key_pressed[i] = key_down[i] && !key_down_prev[i];
    }
}

bool is_key_down(uint8_t scancode) {
    return key_down[scancode];
}

bool is_key_pressed(uint8_t scancode) {
    return key_pressed[scancode];
}
