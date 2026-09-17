#ifndef RSLLINKER_H
#define RSLLINKER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_ENTITIES 32
#define MAX_ASSETS 32
#define MAX_STRING_LEN 64

typedef struct {
    char key[MAX_STRING_LEN];
    char path[MAX_STRING_LEN];
} rsl_asset_t;

typedef struct {
    char name[MAX_STRING_LEN];
    uint32_t screen_width;
    uint32_t screen_height;
    char entry_entity[MAX_STRING_LEN];

    char entities[MAX_ENTITIES][MAX_STRING_LEN];
    uint32_t entity_count;

    rsl_asset_t assets[MAX_ASSETS];
    uint32_t asset_count;
} rsl_manifest_t;

bool parse_rsllink(const char *json_string, rsl_manifest_t *out_manifest);

#endif
