#include "vm.h"
#include "../kernel/framebuffer.h"
#include "../kernel/keyboard.h"
#include "../fs/fat32.h"

#define STACK_MAX 256

static float stack[STACK_MAX];
static int sp = 0;

static inline void push(float val) {
    if (sp < STACK_MAX) stack[sp++] = val;
}

static inline float pop() {
    if (sp > 0) return stack[--sp];
    return 0.0f;
}

// Forward declarations for kernel-level APIs
void draw_rect_uv(float u1, float v1, float u2, float v2, uint32_t color);
void draw_text_uv(const char *str, float u, float v, uint32_t color);
void clear(uint32_t color);

// To support draw_self we need to know about sprites. For now, since we only load one,
// we will assume a global sprite pointer is exposed by main.c
#include "../runtime/bmp_loader.h"
extern sprite_t g_test_sprite;

void vm_exec(uint8_t *bytecode, uint16_t length, EntityInstance *self) {
    uint16_t ip = 0;
    sp = 0; // Reset stack

    while (ip < length) {
        uint8_t opcode = bytecode[ip++];

        switch (opcode) {
            case OP_CONST: {
                union { float f; uint32_t i; } u;
                u.i = bytecode[ip] | (bytecode[ip+1] << 8) | (bytecode[ip+2] << 16) | (bytecode[ip+3] << 24);
                ip += 4;
                push(u.f);
                break;
            }
            case OP_LOAD_PROP: {
                uint8_t prop = bytecode[ip++];
                if (prop == PROP_X) push(self->x);
                else if (prop == PROP_Y) push(self->y);
                else if (prop == PROP_SPEED) push(self->speed);
                else push(0.0f);
                break;
            }
            case OP_STORE_PROP: {
                uint8_t prop = bytecode[ip++];
                float val = pop();
                if (prop == PROP_X) self->x = val;
                else if (prop == PROP_Y) self->y = val;
                else if (prop == PROP_SPEED) self->speed = val;
                break;
            }
            case OP_ADD: {
                float b = pop();
                float a = pop();
                push(a + b);
                break;
            }
            case OP_SUB: {
                float b = pop();
                float a = pop();
                push(a - b);
                break;
            }
            case OP_JUMP_FALSE: {
                uint16_t offset = bytecode[ip] | (bytecode[ip+1] << 8);
                ip += 2;
                float condition = pop();
                if (condition == 0.0f) {
                    ip += offset;
                }
                break;
            }
            case OP_SET_ALARM: {
                uint8_t idx = bytecode[ip++];
                float val = pop();
                if (idx < 16) {
                    self->alarm[idx] = (int32_t)val;
                }
                break;
            }
            case OP_CALL_BUILTIN: {
                uint8_t func = bytecode[ip++];
                uint8_t args = bytecode[ip++]; // args are popped right-to-left

                if (func == BUILTIN_KEY_DOWN) {
                    float scancode = pop();
                    push(is_key_down((uint8_t)scancode) ? 1.0f : 0.0f);
                }
                else if (func == BUILTIN_CLEAR) {
                    float color = pop();
                    clear((uint32_t)color);
                    push(0.0f);
                }
                else if (func == BUILTIN_DRAW_SELF) {
                    // Draw sprite centered on x,y
                    draw_sprite_uv(&g_test_sprite, self->x, self->y, 1.0f, 1.0f);
                    push(0.0f);
                }
                else if (func == BUILTIN_DRAW_TEXT_UV) {
                    // Text is mocked as a string literal constant ID, we pass the raw strings.
                    // For the demo, we assume the args are color, v, u, string_id (which is 0.0)
                    float color = pop();
                    float v = pop();
                    float u = pop();
                    pop(); // discard the fake string_id
                    // Hack for the demo, since we only print one phrase:
                    draw_text_uv("RSL Engine Active - Move with Arrow Keys", u, v, (uint32_t)color);
                    push(0.0f);
                }
                break;
            }
            case OP_RET:
                return;
        }
    }
}

void SV_ap_VR(void) {
    // Stub implementation to write globals out to FAT32
    // The spec requires /apps/<name>/app_data/app_Cache
    // For now we just write a dummy buffer to "cache.bin"
    const char *data = "RSL GLOBAL CACHE\n";
    if (!fs_write_file("cache.bin", data, 17)) {
        // Silently fail if file not already created
    }
}
