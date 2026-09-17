#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

typedef enum {
    OP_CONST = 1,
    OP_LOAD_PROP,
    OP_STORE_PROP,
    OP_LOAD_VAR,
    OP_STORE_VAR,
    OP_ADD,
    OP_SUB,
    OP_CALL_BUILTIN,
    OP_JUMP_FALSE,
    OP_SET_ALARM,
    OP_RET
} OpCode;

typedef enum {
    BUILTIN_CLEAR = 1,
    BUILTIN_DRAW_RECT_UV,
    BUILTIN_DRAW_TEXT_UV,
    BUILTIN_DRAW_SELF,
    BUILTIN_DRAW_SPRITE,
    BUILTIN_KEY_DOWN,
    BUILTIN_KEY_PRESSED
} BuiltinFunc;

typedef enum {
    PROP_X = 1,
    PROP_Y,
    PROP_SPEED,
    PROP_SPRITE_ID
} EntityProp;

// Hardcode Keyboard constants for the VM
#define VM_KEY_UP    72
#define VM_KEY_DOWN  80
#define VM_KEY_LEFT  75
#define VM_KEY_RIGHT 77

#endif
