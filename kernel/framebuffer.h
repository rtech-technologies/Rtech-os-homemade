#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include "limine.h"

void init_framebuffer(struct limine_framebuffer *fb);
void clear(uint32_t color);
void draw_rect_uv(float u1, float v1, float u2, float v2, uint32_t color);
void draw_text_uv(const char *str, float u, float v, uint32_t color);
void swap_buffers(void);

#endif
