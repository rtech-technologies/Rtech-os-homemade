#include "timer.h"
#include "idt.h"
#include "ports.h"

volatile uint32_t tick = 0;

static void timer_callback(registers_t *regs) {
    (void)regs;
    tick++;
}

void init_timer(uint32_t freq) {
    // Register our timer callback.
    register_interrupt_handler(32, &timer_callback);

    // The value we send to the PIT is the value to divide it's input clock
    // (1193180 Hz) by, to get our required frequency. Important to note is
    // that the divisor must be small enough to fit into 16-bits.
    uint32_t divisor = 1193180 / freq;

    // Send the command byte.
    port_byte_out(0x43, 0x36);

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    // Send the frequency divisor.
    port_byte_out(0x40, l);
    port_byte_out(0x40, h);
}

void kernel_sleep(uint32_t ticks) {
    uint32_t end_tick = tick + ticks;
    while(tick < end_tick) {
        __asm__ volatile("hlt");
    }
}
