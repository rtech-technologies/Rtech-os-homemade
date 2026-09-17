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

// Basic US QWERTY mapping for Set 1 Make codes
// Excludes shift states for simplicity in this demo phase
static const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* Ctrl */ 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* LShift */ '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, /* RShift */
    '*', 0, /* Alt */ ' ', 0, /* Caps */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1-F10 */
    0, 0, /* Num/Scroll Lock */
    0, 0, 0, 0, 0, 0, 0, 0, 0, /* Home, Up, PgUp, -, Left, 5, Right, +, End */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

char keyboard_getchar(void) {
    // We can just iterate the key_pressed array to find what character was pressed this frame
    for (int i = 0; i < 128; i++) {
        if (key_pressed[i]) {
            return scancode_to_ascii[i];
        }
    }
    return 0;
}
