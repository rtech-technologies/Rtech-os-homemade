#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "framebuffer.h"
#include "keyboard.h"
#include "memory.h"
#include "../drivers/storage/ahci.h"
#include "../fs/fat32.h"
#include "../runtime/rsllinker.h"
#include "../runtime/bmp_loader.h"

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

    app_arena_init(4); // Allocate some blocks for our file loading

    char rsllink_data[1024] = {0};
    rsl_manifest_t manifest;
    bool has_manifest = false;
    if (fs_read_file("test.rsllink", rsllink_data)) {
        has_manifest = parse_rsllink(rsllink_data, &manifest);
    }

    sprite_t test_sprite;
    bool has_sprite = false;
    if (has_manifest && manifest.asset_count > 0) {
        void *bmp_buf = app_malloc(64000); // 64KB should be plenty for 16x16 test bmp
        if (bmp_buf && fs_read_file(manifest.assets[0].path, bmp_buf)) {
            has_sprite = load_bmp(bmp_buf, &test_sprite);
        }
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

        if (has_manifest) {
            draw_text_uv("Loaded Manifest Name:", 0.3f, 0.30f, 0x0000FF00); // Green
            draw_text_uv(manifest.name, 0.3f, 0.33f, 0x00FFFFFF);
        }

        if (has_sprite) {
            draw_text_uv("Sprite loaded and drawn at UV (0.5, 0.5):", 0.3f, 0.40f, 0x0000FF00);
            draw_sprite_uv(&test_sprite, 0.5f, 0.5f, 4.0f, 4.0f); // Draw it 4x larger
        }

        swap_buffers();
    }
}
