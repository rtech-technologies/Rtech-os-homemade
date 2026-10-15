#include "parser.h"
#include "lexer.h"
#include <stddef.h>

static Token current;
static Token previous;
static bool had_error = false;

static uint8_t *current_chunk;
static uint16_t *current_len;

static void advance_token() {
    previous = current;
    current = lexer_next_token();
    while (current.type == TOK_ERROR) {
        had_error = true;
        current = lexer_next_token();
    }
}

static void consume(TokenType type, const char *msg) {
    if (current.type == type) {
        advance_token();
        return;
    }
    had_error = true;
}

static bool match(TokenType type) {
    if (current.type == type) {
        advance_token();
        return true;
    }
    return false;
}

static void emit_byte(uint8_t byte) {
    if (*current_len < MAX_BYTECODE_SIZE) {
        current_chunk[*current_len] = byte;
        (*current_len)++;
    } else {
        had_error = true; // Buffer overflow
    }
}

static void emit_float(float value) {
    union { float f; uint32_t i; } u;
    u.f = value;
    emit_byte((u.i >> 0) & 0xFF);
    emit_byte((u.i >> 8) & 0xFF);
    emit_byte((u.i >> 16) & 0xFF);
    emit_byte((u.i >> 24) & 0xFF);
}

static void emit_int(uint32_t value) {
    emit_byte((value >> 0) & 0xFF);
    emit_byte((value >> 8) & 0xFF);
    emit_byte((value >> 16) & 0xFF);
    emit_byte((value >> 24) & 0xFF);
}

// Forward declarations for recursive descent
static void expression();
static void parse_block();

static float parse_number(Token t) {
    // Check for hex
    if (t.length >= 3 && t.start[0] == '0' && t.start[1] == 'x') {
        uint32_t val = 0;
        for (int i=2; i<t.length; i++) {
            char c = t.start[i];
            uint32_t nibble = 0;
            if (c >= '0' && c <= '9') nibble = c - '0';
            else if (c >= 'a' && c <= 'f') nibble = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') nibble = 10 + (c - 'A');
            val = (val << 4) | nibble;
        }
        union { float f; uint32_t i; } u;
        u.i = val; // Actually passing raw 32-bit uint inside the float representation
        return u.f;
    }

    float val = 0.0f;
    float fraction = 1.0f;
    bool in_fraction = false;

    for (int i=0; i<t.length; i++) {
        char c = t.start[i];
        if (c == '.') {
            in_fraction = true;
            continue;
        }
        if (!in_fraction) {
            val = val * 10.0f + (c - '0');
        } else {
            fraction *= 0.1f;
            val = val + (c - '0') * fraction;
        }
    }
    return val;
}

static bool string_cmp(const char *s1, int len1, const char *s2) {
    int len2 = 0;
    while(s2[len2]) len2++;
    if (len1 != len2) return false;
    for (int i=0; i<len1; i++) {
        if (s1[i] != s2[i]) return false;
    }
    return true;
}

static void primary() {
    if (match(TOK_NUMBER)) {
        emit_byte(OP_CONST);
        emit_float(parse_number(previous));
        return;
    }

    if (match(TOK_IDENTIFIER)) {
        Token id = previous;

        if (string_cmp(id.start, id.length, "self")) {
            consume(TOK_DOT, "Expected '.' after self");
            consume(TOK_IDENTIFIER, "Expected property name");
            Token prop = previous;
            uint8_t p = 0;
            if (string_cmp(prop.start, prop.length, "x")) p = PROP_X;
            else if (string_cmp(prop.start, prop.length, "y")) p = PROP_Y;
            else if (string_cmp(prop.start, prop.length, "speed")) p = PROP_SPEED;

            emit_byte(OP_LOAD_PROP);
            emit_byte(p);
            return;
        }

        // Built-in function call e.g. key_down(KEY_RIGHT)
        if (match(TOK_LPAREN)) {
            uint8_t func = 0;
            if (string_cmp(id.start, id.length, "key_down")) func = BUILTIN_KEY_DOWN;
            else if (string_cmp(id.start, id.length, "clear")) func = BUILTIN_CLEAR;
            else if (string_cmp(id.start, id.length, "draw_text_uv")) func = BUILTIN_DRAW_TEXT_UV;
            else if (string_cmp(id.start, id.length, "draw_self")) func = BUILTIN_DRAW_SELF;

            int arg_count = 0;
            if (!match(TOK_RPAREN)) {
                do {
                    if (match(TOK_STRING)) {
                        emit_byte(OP_CONST); emit_float(0.0f); // String pointers not really supported, mock passing 0
                    } else if (match(TOK_IDENTIFIER)) {
                        // constants like KEY_RIGHT
                        if (string_cmp(previous.start, previous.length, "KEY_RIGHT")) {
                            emit_byte(OP_CONST); emit_float((float)VM_KEY_RIGHT);
                        } else if (string_cmp(previous.start, previous.length, "KEY_LEFT")) {
                            emit_byte(OP_CONST); emit_float((float)VM_KEY_LEFT);
                        } else if (string_cmp(previous.start, previous.length, "KEY_UP")) {
                            emit_byte(OP_CONST); emit_float((float)VM_KEY_UP);
                        } else if (string_cmp(previous.start, previous.length, "KEY_DOWN")) {
                            emit_byte(OP_CONST); emit_float((float)VM_KEY_DOWN);
                        } else {
                            emit_byte(OP_CONST); emit_float(0.0f);
                        }
                    } else {
                         expression();
                    }
                    arg_count++;
                } while (match(TOK_COMMA));
                consume(TOK_RPAREN, "Expected ')' after arguments");
            }
            emit_byte(OP_CALL_BUILTIN);
            emit_byte(func);
            emit_byte(arg_count);
            return;
        }
    }
}

static void term() {
    primary();
}

static void expression() {
    term();
    while (match(TOK_PLUS) || match(TOK_MINUS)) {
        TokenType op = previous.type;
        term();
        if (op == TOK_PLUS) emit_byte(OP_ADD);
        else emit_byte(OP_SUB);
    }
}

static void statement() {
    if (match(TOK_IF)) {
        expression();
        consume(TOK_COLON, "Expected ':' after if condition");

        emit_byte(OP_JUMP_FALSE);
        int jump_offset = *current_len;
        emit_byte(0); // placeholder
        emit_byte(0);

        parse_block();

        // Patch jump
        int jump_len = *current_len - jump_offset - 2;
        current_chunk[jump_offset] = (jump_len >> 0) & 0xFF;
        current_chunk[jump_offset+1] = (jump_len >> 8) & 0xFF;
        return;
    }

    if (match(TOK_IDENTIFIER)) {
        Token id = previous;

        if (string_cmp(id.start, id.length, "self")) {
            consume(TOK_DOT, "Expected '.'");
            consume(TOK_IDENTIFIER, "Expected property");
            Token prop = previous;
            consume(TOK_ASSIGN, "Expected '='");
            expression();

            uint8_t p = 0;
            if (string_cmp(prop.start, prop.length, "x")) p = PROP_X;
            else if (string_cmp(prop.start, prop.length, "y")) p = PROP_Y;
            else if (string_cmp(prop.start, prop.length, "speed")) p = PROP_SPEED;

            emit_byte(OP_STORE_PROP);
            emit_byte(p);
        } else if (string_cmp(id.start, id.length, "alarm")) {
            consume(TOK_LBRACKET, "Expected '['");
            consume(TOK_NUMBER, "Expected alarm index");
            uint8_t index = (uint8_t)parse_number(previous);
            consume(TOK_RBRACKET, "Expected ']'");
            consume(TOK_ASSIGN, "Expected '='");
            expression();
            emit_byte(OP_SET_ALARM);
            emit_byte(index);
        } else {
            // Function call as statement
            if (match(TOK_LPAREN)) {
                uint8_t func = 0;
                if (string_cmp(id.start, id.length, "clear")) func = BUILTIN_CLEAR;
                else if (string_cmp(id.start, id.length, "draw_text_uv")) func = BUILTIN_DRAW_TEXT_UV;
                else if (string_cmp(id.start, id.length, "draw_self")) func = BUILTIN_DRAW_SELF;

                int arg_count = 0;
                if (!match(TOK_RPAREN)) {
                    do {
                        if (match(TOK_STRING)) {
                            emit_byte(OP_CONST); emit_float(0.0f);
                        } else if (match(TOK_IDENTIFIER)) {
                            emit_byte(OP_CONST); emit_float(0.0f);
                        } else {
                            expression();
                        }
                        arg_count++;
                    } while (match(TOK_COMMA));
                    consume(TOK_RPAREN, "Expected ')'");
                }
                emit_byte(OP_CALL_BUILTIN);
                emit_byte(func);
                emit_byte(arg_count);
            }
        }
    }

    // consume newlines
    while (match(TOK_NEWLINE));
}

static void parse_block() {
    consume(TOK_NEWLINE, "Expected newline after block start");
    // consume(TOK_INDENT, "Expected indentation"); // INDENT isn't emitted right if skipping whitespaces badly
    while (current.type != TOK_DEDENT && current.type != TOK_EOF) {
        int old_len = current.length;
        const char *old_start = current.start;
        TokenType old_type = current.type;

        statement();

        if (current.type == old_type && current.start == old_start && current.length == old_len) {
            advance_token(); // prevent infinite loop if statement doesn't consume
        }
    }
    // consume(TOK_DEDENT, "Expected dedent");
}

static void parse_event(EntityClass *cls) {
    consume(TOK_ON, "Expected 'on'");

    // Multi-word events support
    char evt_name[64] = {0};
    int len = 0;
    while (current.type == TOK_IDENTIFIER || current.type == TOK_LPAREN || current.type == TOK_NUMBER || current.type == TOK_RBRACKET || current.type == TOK_RPAREN) {
        for (int i=0; i<current.length; i++) {
            evt_name[len++] = current.start[i];
        }
        advance_token();
        if (current.type == TOK_IDENTIFIER) evt_name[len++] = ' '; // add space between words
    }

    consume(TOK_COLON, "Expected ':' after event name");

    current_chunk = NULL;
    current_len = NULL;

    if (string_cmp(evt_name, len, "create")) {
        current_chunk = cls->ev_create;
        current_len = &cls->ev_create_len;
    } else if (string_cmp(evt_name, len, "step")) {
        current_chunk = cls->ev_step;
        current_len = &cls->ev_step_len;
    } else if (string_cmp(evt_name, len, "start draw")) {
        current_chunk = cls->ev_start_draw;
        current_len = &cls->ev_start_draw_len;
    } else if (string_cmp(evt_name, len, "draw")) {
        current_chunk = cls->ev_draw;
        current_len = &cls->ev_draw_len;
        cls->has_draw_event = true;
    } else if (string_cmp(evt_name, len, "end draw")) {
        current_chunk = cls->ev_end_draw;
        current_len = &cls->ev_end_draw_len;
    } else if (len >= 7 && evt_name[0] == 'a' && evt_name[1] == 'l' && evt_name[2] == 'a' && evt_name[3] == 'r' && evt_name[4] == 'm') {
        int idx = evt_name[6] - '0';
        current_chunk = cls->ev_alarm[idx];
        current_len = &cls->ev_alarm_len[idx];
    } else {
        // Unknown event, we'll parse block and discard
        uint8_t dummy_chunk[1024];
        uint16_t dummy_len = 0;
        current_chunk = dummy_chunk;
        current_len = &dummy_len;
    }

    parse_block();
    emit_byte(OP_RET); // Ensure event ends with return
}

bool parse_rsl_script(const char *source, EntityClass *out_class) {
    had_error = false;
    lexer_init(source);
    advance_token();

    out_class->ev_create_len = 0;
    out_class->ev_step_len = 0;
    out_class->ev_start_draw_len = 0;
    out_class->ev_draw_len = 0;
    out_class->ev_end_draw_len = 0;
    for(int i=0; i<16; i++) out_class->ev_alarm_len[i] = 0;
    out_class->has_draw_event = false;

    // consume empty lines
    while (match(TOK_NEWLINE));

    consume(TOK_ENTITY, "Expected 'entity'");
    if (current.type == TOK_IDENTIFIER) {
        for(int i=0; i<current.length && i<63; i++) out_class->name[i] = current.start[i];
        out_class->name[current.length] = '\0';
        advance_token();
    }
    consume(TOK_COLON, "Expected ':'");
    consume(TOK_NEWLINE, "Expected newline");

    while (current.type != TOK_DEDENT && current.type != TOK_EOF) {
        while (match(TOK_NEWLINE));
        if (current.type == TOK_ON) {
            parse_event(out_class);
        } else if (current.type != TOK_DEDENT && current.type != TOK_EOF) {
            advance_token(); // skip
        }
    }

    if (!out_class->has_draw_event) {
        // Synthesize a default draw event: draw_self()
        out_class->ev_draw[0] = OP_CALL_BUILTIN;
        out_class->ev_draw[1] = BUILTIN_DRAW_SELF;
        out_class->ev_draw[2] = 0; // 0 args
        out_class->ev_draw[3] = OP_RET;
        out_class->ev_draw_len = 4;
    }

    return !had_error;
}
