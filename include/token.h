#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>


#define for_each_token(t, ta) \
    for (Token* (t) = (ta)->tokens; (t)->value; ++(t))


typedef enum {
    NORMAL=1,
    SINGLE_Q,
    DOUBLE_Q,
} QuoteType;


typedef enum {
    T_WORD=1,
    T_PIPE,
    T_AND,
    T_OR,
    T_SEMI,
    T_AMPER,

    T_REDIR_OUT,
    T_REDIR_OUT_APPEND,
    T_REDIR_ERR_OUT,
    T_REDIR_ERR_OUT_APPEND,
    T_REDIR_IN,
} TokenType;


typedef struct {
    char* value;

    // Only meaningful until expansion; glue_tokens() resets it to NORMAL.
    QuoteType quote_type;

    TokenType token_type;

    // Non-zero if the next token is attached to this one (no whitespace
    // between). Set only when a quote is reached; merged by glue_tokens()
    // after expansion.
    int glued;
} Token;

typedef struct {
    Token* tokens;
    size_t len;
    size_t cap;
} TokenArray;


TokenArray new_token_array(void);

int add_token(TokenArray* t, char* value,
                QuoteType quote_type, TokenType token_type, int glued);

void glue_tokens(TokenArray* ta);

void free_tokens(TokenArray tokens);

#endif
