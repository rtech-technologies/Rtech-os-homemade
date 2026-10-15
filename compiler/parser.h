#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "opcodes.h"

#define MAX_BYTECODE_SIZE 1024

typedef struct {
    uint8_t ev_create[MAX_BYTECODE_SIZE];
    uint16_t ev_create_len;

    uint8_t ev_step[MAX_BYTECODE_SIZE];
    uint16_t ev_step_len;

    uint8_t ev_start_draw[MAX_BYTECODE_SIZE];
    uint16_t ev_start_draw_len;

    uint8_t ev_draw[MAX_BYTECODE_SIZE];
    uint16_t ev_draw_len;

    uint8_t ev_end_draw[MAX_BYTECODE_SIZE];
    uint16_t ev_end_draw_len;

    uint8_t ev_alarm[16][MAX_BYTECODE_SIZE];
    uint16_t ev_alarm_len[16];

    char name[64];
    bool has_draw_event;
} EntityClass;

bool parse_rsl_script(const char *source, EntityClass *out_class);

#endif
