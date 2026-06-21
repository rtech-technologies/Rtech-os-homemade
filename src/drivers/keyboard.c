#include "keyboard.h"
#include "ports.h"
#include "idt.h"
#include "screen.h"

#define SC_MAX 57
#define BACKSPACE 0x0E
#define ENTER 0x1C

#define BUFFER_SIZE 256
static char char_buffer[BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

static const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y',
        'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G',
        'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' '};

static void append_buffer(char c) {
    int next = (buffer_head + 1) % BUFFER_SIZE;
    if (next != buffer_tail) {
        char_buffer[buffer_head] = c;
        buffer_head = next;
    }
}

char kernel_get_char() {
    if (buffer_head == buffer_tail) return 0;
    char c = char_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    return c;
}

static void keyboard_callback(registers_t *regs) {
    (void)regs;
    uint8_t scancode = port_byte_in(0x60);

    if (scancode > SC_MAX) return;
    if (scancode == BACKSPACE) {
        append_buffer('\b');
    } else if (scancode == ENTER) {
        append_buffer('\n');
    } else {
        char letter = sc_ascii[(int)scancode];
        append_buffer(letter);
    }
}

void init_keyboard() {
   register_interrupt_handler(33, keyboard_callback);
}
