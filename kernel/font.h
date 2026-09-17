#ifndef FONT_H
#define FONT_H

#include <stdint.h>

// A simple 8x8 font. Each character is 8 bytes, one byte per row.
// 256 characters * 8 bytes = 2048 bytes.
// Using a basic VGA-style font data.

extern const uint8_t font8x8[2048];

#endif
