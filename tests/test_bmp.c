#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../runtime/bmp_loader.h"

// Define stubs for framebuffer kernel symbols so the test can link
uint64_t fb_width = 800;
uint64_t fb_height = 600;
uint32_t *backbuffer = NULL;

int main() {
    // Generate a minimal valid 2x2 24-bit uncompressed BMP in memory.
    // 2x2 pixels * 3 bytes/pixel = 12 bytes. Padded to 4 byte boundary = 16 bytes per scanline?
    // Wait, 2 * 3 = 6 bytes. Padded to 8 bytes.
    // Stride = ((2 * 24 + 31) / 32) * 4 = (79 / 32) * 4 = 2 * 4 = 8 bytes.

    uint8_t bmp_data[54 + 16] = {0}; // 54 byte header + 16 bytes pixel data

    // File Header
    bmp_data[0] = 'B';
    bmp_data[1] = 'M';
    uint32_t size = 54 + 16;
    memcpy(&bmp_data[2], &size, 4);
    uint32_t offset = 54;
    memcpy(&bmp_data[10], &offset, 4);

    // Info Header
    uint32_t header_size = 40;
    memcpy(&bmp_data[14], &header_size, 4);
    int32_t width = 2;
    memcpy(&bmp_data[18], &width, 4);
    int32_t height = 2; // bottom-up
    memcpy(&bmp_data[22], &height, 4);
    uint16_t planes = 1;
    memcpy(&bmp_data[26], &planes, 2);
    uint16_t bpp = 24;
    memcpy(&bmp_data[28], &bpp, 2);
    uint32_t compression = 0; // BI_RGB
    memcpy(&bmp_data[30], &compression, 4);

    // Write a pixel to verify data pointer logic
    // We just want to test load_bmp parses the header correctly

    sprite_t sprite;
    bool success = load_bmp(bmp_data, &sprite);

    assert(success == true);
    assert(sprite.width == 2);
    assert(sprite.height == 2);
    assert(sprite.bpp == 24);
    assert(sprite.pixel_data == &bmp_data[54]);

    printf("test_bmp: PASS\n");
    return 0;
}
