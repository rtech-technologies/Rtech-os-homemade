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
#include "../vm/engine.h"

sprite_t g_test_sprite;

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

    app_arena_init(16); // Allocate blocks for engine execution

    char rsllink_data[1024] = {0};
    rsl_manifest_t manifest;
    if (fs_read_file("demo.rsllink", rsllink_data)) {
        if (parse_rsllink(rsllink_data, &manifest)) {
            // Load sprite
            if (manifest.asset_count > 0) {
                void *bmp_buf = app_malloc(64000);
                if (bmp_buf && fs_read_file(manifest.assets[0].path, bmp_buf)) {
                    load_bmp(bmp_buf, &g_test_sprite);
                }
            }

            // Load and compile main entity script
            if (manifest.entity_count > 0) {
                char rsl_script[4096] = {0};
                if (fs_read_file(manifest.entities[0], rsl_script)) {
                    engine_init(rsl_script);
                    engine_run();
                }
            }
        }
    }

    // Fallback if engine fails to load
    for (;;) {
        update_keyboard();
        clear(0x000000FF);
        draw_text_uv("Failed to load demo.rsllink or player.rsl", 0.1f, 0.1f, 0x00FFFFFF);
        swap_buffers();
    }
}
