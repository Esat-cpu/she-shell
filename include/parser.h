#ifndef PARSER_H
#define PARSER_H

#include "ast.h"


Node* parse(Token* tokens, size_t token_count, char **error_out);

#endif
