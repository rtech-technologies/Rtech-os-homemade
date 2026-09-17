#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

void init_keyboard(void);
void update_keyboard(void);
bool is_key_down(uint8_t scancode);
bool is_key_pressed(uint8_t scancode);
char keyboard_getchar(void);

#endif
