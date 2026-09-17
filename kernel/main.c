#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "framebuffer.h"

// Define a placeholder for keyboard init and read
extern void init_keyboard(void);
extern void update_keyboard(void);

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static void hcf(void) {
    asm("cli");
    for (;;) {
        asm("hlt");
    }
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == 0) {
        hcf();
    }

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    init_framebuffer(framebuffer);
    init_keyboard();

    // Main loop
    for (;;) {
        update_keyboard();

        // Clear screen to dark blue
        clear(0x00000088);

        // Draw a rectangle
        draw_rect_uv(0.25f, 0.25f, 0.75f, 0.75f, 0x00FF0000); // Red

        // Draw text
        draw_text_uv("RSL-OS 64-bit Kernel Booted!", 0.3f, 0.1f, 0x00FFFFFF); // White

        swap_buffers();
    }
}
