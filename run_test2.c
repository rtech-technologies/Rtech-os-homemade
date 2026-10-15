#include <stdio.h>
#include <assert.h>
#include "compiler/parser.h"

int main() {
    const char *script =
        "entity Player:\n"
        "    on create:\n"
        "        self.x = 0.5\n"
        "    on step:\n"
        "        if key_down(KEY_RIGHT):\n"
        "            self.x = self.x + 0.01\n";

    EntityClass cls;
    bool success = parse_rsl_script(script, &cls);

    printf("Success: %d\n", success);
    return 0;
}
