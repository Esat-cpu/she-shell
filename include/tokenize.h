#ifndef TOKENIZE_H
#define TOKENIZE_H

#include <stddef.h>

#include "token.h"


void tokens_to_str_arr(Token* tokens, char** arr);

void tokenize(const char* command, TokenArray* t);

#endif
