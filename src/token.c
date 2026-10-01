#include <stdlib.h>
#include <string.h>

#include "token.h"
#include "util.h"

#define START_TOKEN_COUNT 10


static void ensure_capacity(TokenArray* t, size_t n) {
    if (t->cap == 0) t->cap = 16;

    while ((t->len + n) >= t->cap) {
        t->cap *= 2;
        t->tokens = srealloc(t->tokens, t->cap * sizeof(Token));
    }
}


TokenArray new_token_array(void) {
    TokenArray t;

    t.tokens = smalloc(START_TOKEN_COUNT * sizeof(Token));
    t.len = 0;
    t.cap = START_TOKEN_COUNT;
    t.tokens[0].value = NULL;

    return t;
}


int add_token(TokenArray* t, char* value,
                QuoteType quote_t, TokenType token_t) {
    // Ignore unquoted empty tokens.
    if (strlen(value) == 0 && quote_t == NORMAL) return 0;

    ensure_capacity(t, 1);

    Token token = {
        .value = sstrdup(value),
        .quote_type = quote_t,
        .token_type = token_t,
    };

    t->tokens[t->len++] = token;
    t->tokens[t->len].value = NULL;
    return 1;
}


void free_tokens(TokenArray ta) {
    for_each_token (t, &ta)
        free(t->value);
    free(ta.tokens);
}
