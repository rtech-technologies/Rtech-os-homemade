#include <stdio.h>
#include <assert.h>
#include "../compiler/parser.h"

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

    assert(success == true);

    // Check auto draw generation
    assert(cls.has_draw_event == false);
    assert(cls.ev_draw_len == 4);
    assert(cls.ev_draw[0] == OP_CALL_BUILTIN);
    assert(cls.ev_draw[1] == BUILTIN_DRAW_SELF);

    printf("test_rsl_compiler: PASS\n");
    return 0;
}
