#ifndef AST_H
#define AST_H

#include "token.h"

/* Redirection List */

typedef struct RedirList {
    TokenType redir_type;
    char* filename;
    int savedfd;
    int targetfd;

    struct RedirList* next;
} RedirList;


void append_to_redir_list(
        RedirList **list, TokenType redir_type, char* filename);

void free_redir_list(RedirList *list);


/* AST Node struct and functions */

typedef struct Node {
    TokenType type;

    union {
        struct {
            int argc;
            char** argv;
            RedirList* redir_list;
            char* command;
        } cmd;

        struct {
            struct Node* left;
            struct Node* right;
        } operator;
    };
} Node;

char* collect_redirection(Token* tokens, size_t *pos, size_t end, Node* node);

Node* make_cmd_node(
        Token* tokens, size_t start, size_t end, char** error_out);

Node* make_operator_node(Token token);

void free_ast(Node* root);

#endif
