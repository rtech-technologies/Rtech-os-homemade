// jsmn handles providing the static implementation when JSMN_STATIC is defined
#define JSMN_STATIC
#include "jsmn.h"

#include "rsllinker.h"

// Basic memory utilities since we are in a freestanding environment without string.h
static void string_copy(char *dest, const char *src, int len) {
    if (len >= MAX_STRING_LEN) {
        len = MAX_STRING_LEN - 1;
    }
    for (int i = 0; i < len; i++) {
        dest[i] = src[i];
    }
    dest[len] = '\0';
}

static bool string_equals(const char *json, jsmntok_t *tok, const char *str) {
    if (tok->type != JSMN_STRING) return false;

    int len = tok->end - tok->start;
    const char *s = json + tok->start;

    int str_len = 0;
    while(str[str_len]) str_len++;

    if (len != str_len) return false;

    for (int i = 0; i < len; i++) {
        if (s[i] != str[i]) return false;
    }
    return true;
}

static uint32_t string_to_uint(const char *json, jsmntok_t *tok) {
    if (tok->type != JSMN_PRIMITIVE) return 0; // Numbers are primitives

    uint32_t val = 0;
    for (int i = tok->start; i < tok->end; i++) {
        if (json[i] >= '0' && json[i] <= '9') {
            val = val * 10 + (json[i] - '0');
        } else {
            break;
        }
    }
    return val;
}

bool parse_rsllink(const char *json_string, rsl_manifest_t *out_manifest) {
    jsmn_parser p;
    jsmntok_t t[128]; // Allow up to 128 tokens

    jsmn_init(&p);

    // We need to pass the length of the string to jsmn_parse
    int len = 0;
    while(json_string[len]) len++;

    int r = jsmn_parse(&p, json_string, len, t, 128);

    if (r < 0 || r < 1 || t[0].type != JSMN_OBJECT) {
        return false;
    }

    // Initialize defaults
    out_manifest->name[0] = '\0';
    out_manifest->screen_width = 0;
    out_manifest->screen_height = 0;
    out_manifest->entry_entity[0] = '\0';
    out_manifest->entity_count = 0;
    out_manifest->asset_count = 0;

    for (int i = 1; i < r; i++) {
        if (string_equals(json_string, &t[i], "name")) {
            string_copy(out_manifest->name, json_string + t[i+1].start, t[i+1].end - t[i+1].start);
            i++;
        } else if (string_equals(json_string, &t[i], "screen_width")) {
            out_manifest->screen_width = string_to_uint(json_string, &t[i+1]);
            i++;
        } else if (string_equals(json_string, &t[i], "screen_height")) {
            out_manifest->screen_height = string_to_uint(json_string, &t[i+1]);
            i++;
        } else if (string_equals(json_string, &t[i], "entry_entity")) {
            string_copy(out_manifest->entry_entity, json_string + t[i+1].start, t[i+1].end - t[i+1].start);
            i++;
        } else if (string_equals(json_string, &t[i], "entities")) {
            if (t[i+1].type == JSMN_ARRAY) {
                int array_size = t[i+1].size;
                int arr_idx = i + 2;
                for (int j = 0; j < array_size && j < MAX_ENTITIES; j++) {
                    string_copy(out_manifest->entities[out_manifest->entity_count++], json_string + t[arr_idx].start, t[arr_idx].end - t[arr_idx].start);
                    arr_idx++;
                }
                i += array_size + 1; // Skip the array elements
            } else {
                i++; // not an array? skip value
            }
        } else if (string_equals(json_string, &t[i], "assets")) {
            if (t[i+1].type == JSMN_OBJECT) {
                int keys = t[i+1].size;
                int obj_idx = i + 2;
                for (int j = 0; j < keys && out_manifest->asset_count < MAX_ASSETS; j++) {
                    // key
                    string_copy(out_manifest->assets[out_manifest->asset_count].key, json_string + t[obj_idx].start, t[obj_idx].end - t[obj_idx].start);
                    obj_idx++;
                    // value
                    string_copy(out_manifest->assets[out_manifest->asset_count].path, json_string + t[obj_idx].start, t[obj_idx].end - t[obj_idx].start);
                    obj_idx++;

                    out_manifest->asset_count++;
                }
                i = obj_idx - 1;
            } else {
                i++;
            }
        } else {
            // Unknown key, skip its value
            // Skipping complex objects is hard in jsmn without recursion or a counter.
            // For this simple schema, we assume values are primitive/strings.
            // If it's an object/array, we would technically need to jump over all its children.
            // We do a naive skip for primitive values here.
            i++;
        }
    }

    return true;
}
