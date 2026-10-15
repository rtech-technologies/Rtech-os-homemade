#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdbool.h>
#include "../compiler/opcodes.h"
#include "../compiler/parser.h"

// RSL Entity Instance State
typedef struct {
    float x;
    float y;
    float speed;
    int32_t alarm[16];

    // For rendering
    int sprite_id;
    bool active;

    EntityClass *cls;
} EntityInstance;

void vm_exec(uint8_t *bytecode, uint16_t length, EntityInstance *self);

// Stubs for globals
void SV_ap_VR(void);

#endif
