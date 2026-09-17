#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "framebuffer.h"
#include "keyboard.h"
#include "memory.h"
#include "../drivers/storage/ahci.h"
#include "../fs/fat32.h"

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
    init_memory();
    init_storage();
    init_fat32();

    char typing_buffer[256] = {0};
    int type_idx = 0;

    char file_content[512] = {0};
    if (fs_read_file("test.txt", file_content)) {
        // null-terminate just in case
        file_content[511] = '\0';
    } else {
        file_content[0] = 'F'; file_content[1] = 'A'; file_content[2] = 'I'; file_content[3] = 'L'; file_content[4] = '\0';
    }

    // Main loop
    for (;;) {
        update_keyboard();

        char c = keyboard_getchar();
        if (c != 0) {
            if (c == '\b') {
                if (type_idx > 0) {
                    type_idx--;
                    typing_buffer[type_idx] = '\0';
                }
            } else if (type_idx < 254) {
                typing_buffer[type_idx] = c;
                type_idx++;
                typing_buffer[type_idx] = '\0';
            }
        }

        // Clear screen to dark blue
        clear(0x00000088);

        // Draw a rectangle
        draw_rect_uv(0.25f, 0.25f, 0.75f, 0.75f, 0x00FF0000); // Red

        // Draw text
        draw_text_uv("RSL-OS 64-bit Kernel Booted!", 0.3f, 0.1f, 0x00FFFFFF); // White

        draw_text_uv("File Content (test.txt):", 0.3f, 0.15f, 0x0000FF00); // Green
        draw_text_uv(file_content, 0.3f, 0.18f, 0x00FFFFFF);

        draw_text_uv("Type here:", 0.3f, 0.22f, 0x00FFFF00); // Yellow
        draw_text_uv(typing_buffer, 0.3f, 0.25f, 0x00FFFFFF); // White typed text

        swap_buffers();
    }
}
