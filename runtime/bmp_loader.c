#include "bmp_loader.h"
#include "../kernel/framebuffer.h"

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} bmp_file_header_t;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} bmp_info_header_t;
#pragma pack(pop)

bool load_bmp(const void *file_buffer, sprite_t *out_sprite) {
    if (!file_buffer || !out_sprite) return false;

    const uint8_t *buf = (const uint8_t *)file_buffer;
    bmp_file_header_t *file_hdr = (bmp_file_header_t *)buf;

    if (file_hdr->bfType != 0x4D42) { // "BM"
        return false;
    }

    bmp_info_header_t *info_hdr = (bmp_info_header_t *)(buf + sizeof(bmp_file_header_t));

    if (info_hdr->biCompression != 0) { // We only support uncompressed (BI_RGB)
        return false;
    }

    if (info_hdr->biBitCount != 24 && info_hdr->biBitCount != 32) {
        return false;
    }

    out_sprite->width = (uint32_t)info_hdr->biWidth;
    // height can be negative indicating top-down DIB.
    if (info_hdr->biHeight < 0) {
        out_sprite->height = (uint32_t)(-info_hdr->biHeight);
    } else {
        out_sprite->height = (uint32_t)info_hdr->biHeight;
    }

    out_sprite->bpp = info_hdr->biBitCount;
    out_sprite->pixel_data = (uint8_t *)(buf + file_hdr->bfOffBits);

    return true;
}

// Framebuffer resolution helpers (these should ideally be exposed by framebuffer.h,
// but we'll declare them here to link against the kernel)
extern uint64_t fb_width;
extern uint64_t fb_height;
extern uint32_t *backbuffer;

static inline void put_pixel(uint64_t x, uint64_t y, uint32_t color) {
    if (x < fb_width && y < fb_height && backbuffer) {
        backbuffer[y * fb_width + x] = color;
    }
}

void draw_sprite_uv(const sprite_t *sprite, float u, float v, float scale_x, float scale_y) {
    if (!sprite || !sprite->pixel_data || !backbuffer) return;

    // Convert starting UV to pixel coords
    uint64_t start_x = (uint64_t)(u * fb_width);
    uint64_t start_y = (uint64_t)(v * fb_height);

    uint32_t scaled_width = (uint32_t)((float)sprite->width * scale_x);
    uint32_t scaled_height = (uint32_t)((float)sprite->height * scale_y);

    // Row stride handles Windows BMP 4-byte boundary padding
    uint32_t row_stride = ((sprite->width * sprite->bpp + 31) / 32) * 4;

    for (uint32_t sy = 0; sy < scaled_height; sy++) {
        for (uint32_t sx = 0; sx < scaled_width; sx++) {

            // Nearest neighbor sampling
            uint32_t src_x = (uint32_t)((float)sx / scale_x);
            uint32_t src_y = (uint32_t)((float)sy / scale_y);

            if (src_x >= sprite->width) src_x = sprite->width - 1;
            if (src_y >= sprite->height) src_y = sprite->height - 1;

            // BMPs are usually stored bottom-up, unless biHeight was negative
            // We assume bottom-up here since it's the standard for 24/32-bit BMPs.
            // If top-down is needed, a flag should be stored in sprite_t during load.
            uint32_t bmp_y = (sprite->height - 1) - src_y;

            uint8_t *pixel_ptr = sprite->pixel_data + (bmp_y * row_stride) + (src_x * (sprite->bpp / 8));

            uint8_t b = pixel_ptr[0];
            uint8_t g = pixel_ptr[1];
            uint8_t r = pixel_ptr[2];
            uint8_t a = 255;

            if (sprite->bpp == 32) {
                a = pixel_ptr[3];
            } else if (sprite->bpp == 24) {
                // Check magenta color key
                if (r == 255 && g == 0 && b == 255) {
                    a = 0; // Transparent
                }
            }

            if (a > 0) { // Skip fully transparent pixels
                // Minimal blending (or just solid drawing for this demo)
                // For simplicity, we just draw solid.
                uint32_t final_color = (a << 24) | (r << 16) | (g << 8) | b;
                put_pixel(start_x + sx, start_y + sy, final_color);
            }
        }
    }
}
