#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../runtime/rsllinker.h"

int main() {
    const char *json =
        "{\n"
        "  \"name\": \"TestGame\",\n"
        "  \"screen_width\": 800,\n"
        "  \"screen_height\": 600,\n"
        "  \"entry_entity\": \"Main\",\n"
        "  \"entities\": [\"main.rsl\", \"player.rsl\"],\n"
        "  \"assets\": {\n"
        "    \"spr_test\": \"test.bmp\"\n"
        "  }\n"
        "}";

    rsl_manifest_t manifest;
    bool success = parse_rsllink(json, &manifest);

    assert(success == true);
    assert(strcmp(manifest.name, "TestGame") == 0);
    assert(manifest.screen_width == 800);
    assert(manifest.screen_height == 600);
    assert(strcmp(manifest.entry_entity, "Main") == 0);

    assert(manifest.entity_count == 2);
    assert(strcmp(manifest.entities[0], "main.rsl") == 0);
    assert(strcmp(manifest.entities[1], "player.rsl") == 0);

    assert(manifest.asset_count == 1);
    assert(strcmp(manifest.assets[0].key, "spr_test") == 0);
    assert(strcmp(manifest.assets[0].path, "test.bmp") == 0);

    printf("test_rsllinker: PASS\n");
    return 0;
}
