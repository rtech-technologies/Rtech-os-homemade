#ifndef UTILS_H
#define UTILS_H

#include "types.h"

void memory_copy(uint8_t *source, uint8_t *dest, int nbytes);
void memory_set(uint8_t *dest, uint8_t val, uint32_t len);
void int_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(char s[]);

#endif
