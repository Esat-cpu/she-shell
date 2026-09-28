#ifndef PARSER_H
#define PARSER_H

#include "ast.h"


typedef struct {
    Token* tokens;
    size_t pos;
    size_t end;
    char* error_message;
} Parser;


Node* parse(Token* tokens, size_t token_count, char **error_out);

#endif
