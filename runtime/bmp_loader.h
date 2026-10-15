#ifndef BMP_LOADER_H
#define BMP_LOADER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint16_t bpp;
    uint8_t *pixel_data;
} sprite_t;

bool load_bmp(const void *file_buffer, sprite_t *out_sprite);
void draw_sprite_uv(const sprite_t *sprite, float u, float v, float scale_x, float scale_y);

#endif
