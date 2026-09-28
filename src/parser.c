#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "parser.h"
#include "ast.h"
#include "token.h"

/*
* Grammar (lowest to highest precedence):
*       sequence    = logical, { ";", logical }, [ ";" ]
*       logical     = async_list, { ("&&" | "||"), async_list }
*       async_list  = pipe, { "&", pipe }, [ "&" ]
*       pipe        = command, { "|", command }
*       command     = { redirection }, word, { redirection | word }
*       redirection = redir_operator, word
*/

// Returns true if this token ends a command segment (|, &&, ||, ;, &).
// Redirection tokens are NOT terminators -- they're part of the
// command itself and are handled inside make_cmd_node.
static bool is_command_terminator(Token token) {
    return token.token_type == T_PIPE
        || token.token_type == T_OR
        || token.token_type == T_AND
        || token.token_type == T_SEMI
        || token.token_type == T_AMPER;
}


static Node* parse_command(Parser* p) {
    size_t start = p->pos;

    while (p->pos < p->end && !is_command_terminator(p->tokens[p->pos]))
        (p->pos)++;

    size_t end = p->pos;

    Node* node = make_cmd_node(p->tokens, start, end, &p->error_message);

    return node;
}


static Node* parse_pipe(Parser* p) {
    Node* left = parse_command(p);
    if (!left) return NULL;

    while (p->pos < p->end && p->tokens[p->pos].token_type == T_PIPE) {
        Node* op = make_operator_node(p->tokens[p->pos]);
        p->pos++;

        Node* right = parse_command(p);
        if (!right) {
            free(op);
            free_ast(left);
            return NULL;
        }

        op->operator.left = left;
        op->operator.right = right;

        left = op;
    }

    return left;
}


// Asynchronous list
// A trailing '&' is valid. In that case, the operator node's
// right arm will be NULL.
static Node* parse_async_list(Parser* p) {
    Node* left = parse_pipe(p);
    if (!left) return NULL;

    while (p->pos < p->end && p->tokens[p->pos].token_type == T_AMPER) {
        Node* op = make_operator_node(p->tokens[p->pos]);
        p->pos++;

        Node* right = NULL;

        if (p->pos < p->end) {
            right = parse_pipe(p);
            if (!right) {
                free(op);
                free_ast(left);
                return NULL;
            }
        }

        op->operator.left = left;
        op->operator.right = right;

        left = op;
    }

    return left;
}


static Node* parse_logical(Parser* p) {
    Node* left = parse_async_list(p);
    if (!left) return NULL;

    while (p->pos < p->end && (p->tokens[p->pos].token_type == T_AND
                            || p->tokens[p->pos].token_type == T_OR)) {
        Node* op = make_operator_node(p->tokens[p->pos]);
        p->pos++;

        Node* right = parse_async_list(p);
        if (!right) {
            free(op);
            free_ast(left);
            return NULL;
        }

        op->operator.left = left;
        op->operator.right = right;

        left = op;
    }

    return left;
}


// A trailing ';' is valid. In that case, the operator node's
// right arm will be NULL.
static Node* parse_sequence(Parser* p) {
    Node* left = parse_logical(p);
    if (!left) return NULL;

    while (p->pos < p->end && p->tokens[p->pos].token_type == T_SEMI) {
        Node* op = make_operator_node(p->tokens[p->pos]);
        p->pos++;

        Node* right = NULL;

        if (p->pos < p->end) {
            right = parse_logical(p);
            if (!right) {
                free(op);
                free_ast(left);
                return NULL;
            }
        }

        op->operator.left = left;
        op->operator.right = right;

        left = op;
    }

    return left;
}


// Entry point: parses a full token stream into an AST. On success
// returns the root node and leaves *error_message untouched (NULL).
// On a parse error, returns NULL and writes a heap-allocated message
// to *error_message -- the caller is responsible for freeing it.
// 'tokens' array must have at least one token.
Node* parse(Token* tokens, size_t token_count, char **error_out) {
    Parser p = {
        .tokens = tokens,
        .pos = 0,
        .end = token_count,
        .error_message = NULL,
    };

    Node* root = parse_sequence(&p);
    *error_out = p.error_message;

    return root;
}
