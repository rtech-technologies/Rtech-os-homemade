#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"

/**
 * KERNEL STARTING POINT
 * ---------------------
 * This is where the magic happens.
 * The system has been initialized with GDT, IDT, and basic drivers.
 */
void kernel_main() {
    /* ----------------------------------------------------------------------
     * 1. SYSTEM INITIALIZATION (Low-level drivers)
     * ---------------------------------------------------------------------- */
    init_gdt();          // Setup memory segments
    init_idt();          // Setup interrupt handling

    clear_screen();      // Prepare VGA Video memory

    init_timer(50);      // Start System Timer (IRQ 0) at 50Hz
    init_keyboard();     // Start Keyboard Driver (IRQ 1)

    /* Enable Interrupts - The system is now alive and responding to events */
    __asm__ volatile("sti");


    /* ----------------------------------------------------------------------
     * 2. YOUR USER INTERFACE (Playground)
     * ----------------------------------------------------------------------
     * You can now use any of the driver functions defined in headers like
     * screen.h, keyboard.h, and timer.h.
     *
     * Example: kprint("Hello World!");
     */

    kprint("Rtech-OS Kernel Foundation Loaded.\n");
    kprint("Type something to see it on screen, or start building your UI!\n");
    kprint("> ");

    while(1) {
        /* Example: Simple Echo Loop */
        char c = kernel_get_char();
        if (c != 0) {
            if (c == '\b') {
                kprint_backspace();
            } else if (c == '\n') {
                kprint("\n> ");
            } else {
                char str[2] = {c, 0};
                kprint(str);
            }
        }

        /* You can also use kernel_sleep(ticks) to timing-based logic */
    }
}
