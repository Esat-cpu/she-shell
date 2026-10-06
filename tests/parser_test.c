#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "test_lib.h"
#include "token.h"
#include "tokenize.h"
#include "parser.h"

#define MAX_ARGS 64
#define OUT_BUF 4096


TokenArray ta;
Node* root = NULL;
char out[OUT_BUF] = "";


typedef struct {
    const char* command;
    const char* expected_ast; // NULL means parse() should return NULL
} TestCase;


typedef struct {
    const char* command;          // must make parse() return NULL
    const char* expected_message; // message written to *error_message
} ErrorCase;


/* Helper Functions */

static const char*
op_str(TokenType type) {
    switch (type) {
        case T_PIPE: return "PIPE";
        case T_AND:  return "AND";
        case T_OR:   return "OR";
        case T_SEMI: return "SEMI";

        case T_REDIR_OUT:            return ">";
        case T_REDIR_OUT_APPEND:     return ">>";
        case T_REDIR_ERR_OUT:        return "2>";
        case T_REDIR_ERR_OUT_APPEND: return "2>>";

        default: return "?";
    }
}


static void
arr_to_str(char** arr, char *out) {
    if (!arr[0])
        return;

    strcat(out, arr[0]);

    for (size_t i = 1; arr[i]; ++i) {
        strncat(out, " ", OUT_BUF - strlen(out) - 1);
        strncat(out, arr[i], OUT_BUF - strlen(out) - 1);
    }
}


static void
append_redir_operators(RedirList* list, char *out) {
    while (list) {
        strncat(out, " ", OUT_BUF - strlen(out) - 1);
        strncat(out, op_str(list->redir_type), OUT_BUF - strlen(out) - 1);
        strncat(out, list->filename, OUT_BUF - strlen(out) - 1);
        list = list->next;
    }
}


static void
ast_to_str(Node* node, char *out) {
    out[0] = '\0';

    if (!node) {
        strcpy(out, "NULL");
        return;
    }

    if (node->type == T_WORD) {
        if (!node->cmd.argv[0] && !node->cmd.redir_list) {
            strcpy(out, "<>");
        }
        else {
            strcat(out, "<");

            arr_to_str(node->cmd.argv, out);
            append_redir_operators(node->cmd.redir_list, out);

            size_t len = strlen(out);
            if (len < OUT_BUF -1) {
                out[len] = '>';
                out[len+1] = '\0';
            }
        }
    }
    else {
        size_t size = OUT_BUF;
        char left[size];
        char right[size];
        ast_to_str(node->operator.left, left);
        ast_to_str(node->operator.right, right);
        snprintf(out, size,
                "%s (%s, %s)", op_str(node->type), left, right);
    }
}


static void
command_to_parsed_str(const char* command, char *out, char **error_out) {
    char *lex_er;
    tokenize(command, &ta, &lex_er);

    char *error_message = NULL;

    root = parse(ta.tokens, ta.len, &error_message);

    if (error_out)
        *error_out = error_message;
    else
        free(error_message);

    ast_to_str(root, out);
}


/* Tests */

static void
test_parse_command() {
    TestCase c = {
        "echo helo world",
        "<echo helo world>",
    };

    command_to_parsed_str(c.command, out, NULL);

    ASSERT_EQ(out, c.expected_ast);

    free_ast(root);
    free_tokens(ta);
}


static void
test_parse_redirection_operators() {
    TestCase c1 = {
        "echo > hello.txt",
        "<echo >hello.txt>"
    };
    TestCase c2 = {
        "> file echo test",
        "<echo test >file>",
    };
    TestCase c3 = {
        "echo 2> err test >> file",
        "<echo test 2>err >>file>",
    };
    TestCase c4 = {
        "cat test > file 2>> err",
        "<cat test >file 2>>err>",
    };

    command_to_parsed_str(c1.command, out, NULL);
    ASSERT_EQ(out, c1.expected_ast);

    free_ast(root);
    free_tokens(ta);

    command_to_parsed_str(c2.command, out, NULL);
    ASSERT_EQ(out, c2.expected_ast);

    free_ast(root);
    free_tokens(ta);

    command_to_parsed_str(c3.command, out, NULL);
    ASSERT_EQ(out, c3.expected_ast);

    free_ast(root);
    free_tokens(ta);

    command_to_parsed_str(c4.command, out, NULL);
    ASSERT_EQ(out, c4.expected_ast);

    free_ast(root);
    free_tokens(ta);
}


static void
test_parse_logical_operators() {
    TestCase c = {
        "echo helo && echo yes || echo no && echo test",
        "AND (OR (AND (<echo helo>, <echo yes>), <echo no>), <echo test>)",
    };

    command_to_parsed_str(c.command, out, NULL);

    ASSERT_EQ(out, c.expected_ast);

    free_ast(root);
    free_tokens(ta);
}


static void
test_parse_pipe_operator() {
    TestCase c = {
        "echo helo | cat | tr a-z A-Z | test",
        "PIPE (PIPE (PIPE (<echo helo>, <cat>), <tr a-z A-Z>), <test>)",
    };

    command_to_parsed_str(c.command, out, NULL);

    ASSERT_EQ(out, c.expected_ast);

    free_ast(root);
    free_tokens(ta);
}


static void
test_parse_semicolons() {
    TestCase c = {
        "echo helo; cat test ; echo helo;",
        "SEMI (SEMI (SEMI (<echo helo>, <cat test>), <echo helo>), NULL)",
    };

    command_to_parsed_str(c.command, out, NULL);

    ASSERT_EQ(out, c.expected_ast);

    free_ast(root);
    free_tokens(ta);
}


static void
test_parse_mix() {
    TestCase c1 = {
        "echo hello | cat && echo yes || echo no; test test && echo | cat",

        "SEMI (OR (AND (PIPE (<echo hello>, <cat>), <echo yes>), "
        "<echo no>), AND (<test test>, PIPE (<echo>, <cat>)))",
    };

    TestCase c2 = {
        "cat >file test && >> ok.txt echo yes | tr test 2> err;",
        "SEMI (AND (<cat test >file>, PIPE (<echo yes >>ok.txt>, <tr test 2>err>)), NULL)",
    };

    command_to_parsed_str(c1.command, out, NULL);
    ASSERT_EQ(out, c1.expected_ast);

    free_ast(root);
    free_tokens(ta);

    command_to_parsed_str(c2.command, out, NULL);
    ASSERT_EQ(out, c2.expected_ast);

    free_ast(root);
    free_tokens(ta);
}


static void
test_parse_error_message() {
    ErrorCase c1 = {
        "; echo helo",
        "Parse error near ';'",
    };
    ErrorCase c2 = {
        "echo helo >",
        "Parse error near '>'",
    };
    ErrorCase c3 = {
        "echo && | cat",
        "Parse error near '&&'",
    };

    char* err = NULL;

    command_to_parsed_str(c1.command, out, &err);
    ASSERT_EQ(err, c1.expected_message);

    free(err);
    free_ast(root);
    free_tokens(ta);

    command_to_parsed_str(c2.command, out, &err);
    ASSERT_EQ(err, c2.expected_message);

    free(err);
    free_ast(root);
    free_tokens(ta);

    command_to_parsed_str(c3.command, out, &err);
    ASSERT_EQ(err, c3.expected_message);

    free(err);
    free_ast(root);
    free_tokens(ta);
}


int
main() {
    RUN_TESTS(
        "Parser",
        test_parse_command,
        test_parse_redirection_operators,
        test_parse_logical_operators,
        test_parse_pipe_operator,
        test_parse_semicolons,
        test_parse_mix,
        test_parse_error_message,
    );
}
