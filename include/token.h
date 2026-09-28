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
    QuoteType quote_type;
    TokenType token_type;
} Token;

typedef struct {
    Token* tokens;
    size_t len;
    size_t cap;
} TokenArray;


TokenArray new_token_array(void);

int add_token(TokenArray* t, char* value,
                QuoteType quote_type, TokenType token_type);

void free_tokens(TokenArray tokens);

#endif
