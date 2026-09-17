#include "framebuffer.h"
#include "font.h"
#include <stddef.h>

static struct limine_framebuffer *g_fb = NULL;
static uint32_t *backbuffer = NULL;
static uint64_t fb_width = 0;
static uint64_t fb_height = 0;
static uint64_t fb_pitch = 0;

// Since we have no malloc yet, we will place the backbuffer in a statically allocated BSS section.
// A common resolution is 1920x1080. We'll allocate enough for 4K just to be safe.
// 3840 * 2160 * 4 bytes = ~33 MB.
#define MAX_FB_WIDTH 3840
#define MAX_FB_HEIGHT 2160
static uint32_t g_backbuffer[MAX_FB_WIDTH * MAX_FB_HEIGHT];

void init_framebuffer(struct limine_framebuffer *fb) {
    if (!fb) return;
    g_fb = fb;
    fb_width = fb->width;
    fb_height = fb->height;
    fb_pitch = fb->pitch; // in bytes

    backbuffer = g_backbuffer;
}

void clear(uint32_t color) {
    if (!backbuffer) return;
    for (uint64_t i = 0; i < fb_width * fb_height; i++) {
        backbuffer[i] = color;
    }
}

// Convert normalized coordinates (0.0 to 1.0) to pixel coordinates
static inline void uv_to_pixel(float u, float v, uint64_t *x, uint64_t *y) {
    *x = (uint64_t)(u * fb_width);
    *y = (uint64_t)(v * fb_height);

    if (*x >= fb_width) *x = fb_width - 1;
    if (*y >= fb_height) *y = fb_height - 1;
}

void draw_rect_uv(float u1, float v1, float u2, float v2, uint32_t color) {
    if (!backbuffer) return;

    uint64_t x1, y1, x2, y2;
    uv_to_pixel(u1, v1, &x1, &y1);
    uv_to_pixel(u2, v2, &x2, &y2);

    if (x1 > x2) { uint64_t t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { uint64_t t = y1; y1 = y2; y2 = t; }

    for (uint64_t y = y1; y <= y2; y++) {
        for (uint64_t x = x1; x <= x2; x++) {
            backbuffer[y * fb_width + x] = color;
        }
    }
}

static void draw_char(char c, uint64_t px, uint64_t py, uint32_t color) {
    if (!backbuffer) return;

    const uint8_t *glyph = &font8x8[(uint8_t)c * 8];
    for (uint64_t y = 0; y < 8; y++) {
        for (uint64_t x = 0; x < 8; x++) {
            if (glyph[y] & (1 << (7 - x))) {
                if (px + x < fb_width && py + y < fb_height) {
                    backbuffer[(py + y) * fb_width + (px + x)] = color;
                }
            }
        }
    }
}

void draw_text_uv(const char *str, float u, float v, uint32_t color) {
    if (!backbuffer) return;

    uint64_t x, y;
    uv_to_pixel(u, v, &x, &y);

    while (*str) {
        if (*str == '\n') {
            y += 8;
            uv_to_pixel(u, v, &x, NULL); // reset x
        } else {
            draw_char(*str, x, y, color);
            x += 8;
        }
        str++;
    }
}

void swap_buffers(void) {
    if (!g_fb || !backbuffer) return;

    uint8_t *dst = (uint8_t *)g_fb->address;
    uint32_t *src = backbuffer;

    // Copy row by row using the pitch (bytes per scanline)
    for (uint64_t y = 0; y < fb_height; y++) {
        // We can optimize the row copy using 64-bit word copies if desired,
        // but since width*4 might not be the pitch exactly, we copy exactly width * 4 bytes per row.
        uint64_t *dst_row = (uint64_t *)(dst + y * fb_pitch);
        uint64_t *src_row = (uint64_t *)(src + y * fb_width);

        uint64_t qwords = (fb_width * 4) / 8;
        for (uint64_t i = 0; i < qwords; i++) {
            dst_row[i] = src_row[i];
        }

        // Handle remaining bytes if width * 4 is not a multiple of 8
        uint64_t remaining_bytes = (fb_width * 4) % 8;
        if (remaining_bytes > 0) {
            uint8_t *dst_tail = (uint8_t *)dst_row + qwords * 8;
            uint8_t *src_tail = (uint8_t *)src_row + qwords * 8;
            for (uint64_t i = 0; i < remaining_bytes; i++) {
                dst_tail[i] = src_tail[i];
            }
        }
    }
}
