#include <stdlib.h>
#include <string.h>

#include "token.h"
#include "util.h"
#include "str_util.h"

#define START_TOKEN_COUNT 10


static void ensure_capacity(TokenArray* t, size_t n) {
    size_t cap = t->_cap;

    while ((t->len + n) >= cap)
        cap *= 2;

    if (cap != t->_cap) {
        t->tokens = srealloc(t->tokens, cap * sizeof(Token));
        t->_cap = cap;
    }
}


TokenArray new_token_array(void) {
    TokenArray t;

    t.tokens = smalloc(START_TOKEN_COUNT * sizeof(Token));
    t.len = 0;
    t._cap = START_TOKEN_COUNT;
    t.tokens[0].value = NULL;  // terminator

    return t;
}


// Invariant: the last token's value is always NULL (terminator).
int add_token(TokenArray* t, char* value,
                QuoteType quote_t, TokenType token_t, int glued) {
    // Ignore unquoted empty tokens.
    if (strlen(value) == 0 && quote_t == NORMAL) return 0;

    ensure_capacity(t, 1);

    Token token = {
        .value = sstrdup(value),
        .quote_type = quote_t,
        .token_type = token_t,
        .glued = glued,
    };

    t->tokens[t->len++] = token;
    t->tokens[t->len].value = NULL;
    return 1;
}


// Call this after expansion.
void glue_tokens(TokenArray* ta) {
    if (ta->len == 0) return;

    TokenArray t_new = new_token_array();
    String str = new_string();

    Token *cur = &ta->tokens[0];

    while (cur->value) {
        if (cur->token_type != T_WORD) {
            add_token(&t_new, cur->value, cur->quote_type, cur->token_type, 0);
            cur++;
            continue;
        }

        add_slice_to_str(&str, cur->value);

        while (cur->glued
                && (cur+1)->value && (cur+1)->token_type == T_WORD
        ) {
            add_slice_to_str(&str, (++cur)->value);
        }

        add_token(&t_new, str.data, cur->quote_type, T_WORD, 0);
        clear_str(&str);
        cur++;
    }

    free(str.data);
    free_tokens(*ta);
    *ta = t_new;
}


void free_tokens(TokenArray ta) {
    for_each_token (t, &ta)
        free(t->value);
    free(ta.tokens);
}
