#include <stdio.h>
#include <stdlib.h>
#include "compiler/lexer.h"

int main() {
    const char *script =
        "entity Player:\n"
        "    on create:\n"
        "        self.x = 0.5\n"
        "    on step:\n"
        "        if key_down(KEY_RIGHT):\n"
        "            self.x = self.x + 0.01\n";

    lexer_init(script);
    Token t;
    int count = 0;
    do {
        t = lexer_next_token();
        printf("Token: %s (len: %d, line: %d) text: '%.*s'\n", token_type_name(t.type), t.length, t.line, t.length, t.start);
        count++;
        if(count > 50) break;
    } while (t.type != TOK_EOF && t.type != TOK_ERROR);
    return 0;
}
