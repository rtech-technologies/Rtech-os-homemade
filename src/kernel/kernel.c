#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"

void kernel_main() {
    // 1. Initialize the GDT
    init_gdt();

    // 2. Initialize the IDT
    init_idt();

    // 3. Initialize the screen
    clear_screen();
    kprint("Rtech-OS Kernel Loaded.\n");
    kprint("Initializing drivers...\n");

    // 4. Initialize the PIT (System Timer) at 50Hz
    init_timer(50);
    kprint("Timer initialized.\n");

    // 5. Initialize the Keyboard
    init_keyboard();
    kprint("Keyboard initialized.\n");

    // Enable interrupts
    __asm__ volatile("sti");

    kprint("System ready.\n");
    kprint("--------------------------------------------------\n");

    /*
     * CLEAN UI ENTRY POINT HOOK:
     * This area is designated for the User Interface implementation.
     * All low-level drivers (Screen, Keyboard, Timer, GDT, IDT) are fully active.
     */

    while(1) {
        char c = kernel_get_char();
        if (c != 0) {
            if (c == '\b') {
                kprint_backspace();
            } else {
                char str[2] = {c, 0};
                kprint(str);
            }
        }
    }
}
