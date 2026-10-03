#include <stdlib.h>

#include "test_lib.h"
#include "token.h"
#include "tokenize.h"
#include "expansion.h"
#include "shell.h"

#define MAX_ARGS 64


typedef struct {
    const char* command;
    char* expected_args[MAX_ARGS];
} TestCase;


TokenArray ta;
char* arr[MAX_ARGS];
char *msg = NULL;


static void
set_up() {
    setenv("TEST_ENV_VAR", "test", 1);
    setenv("TEST_ENV_VAR_SPACE", "test word", 1);

    static char* argv[3] = {"shell", "test_arg", NULL};

    shell.name = "shell";
    shell.argc = 2;
    shell.argv = argv;
}


static void
test_param_expansion_with_home_var() {
    char* env_home = getenv("HOME");
    if (!env_home) env_home = "";

    TestCase c = {
        "echo $HOME",
        {"echo", env_home, NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_unquoted() {
    TestCase c = {
        "echo $TEST_ENV_VAR $TEST_ENV_VAR_SPACE",
        {"echo", "test", "test", "word", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_double_quoted() {
    TestCase c = {
        "echo \"$TEST_ENV_VAR\" \"$TEST_ENV_VAR_SPACE\"",
        {"echo", "test", "test word", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_single_quoted() {
    TestCase c = {
        "echo '$TEST_ENV_VAR'",
        {"echo", "$TEST_ENV_VAR", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_with_slash() {
    TestCase c = {
        "echo /hello/$TEST_ENV_VAR/world /h/$TEST_ENV_VAR_SPACE/w",
        {"echo", "/hello/test/world", "/h/test", "word/w", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_exit_code() {
    shell.exit_code = 42;

    TestCase c = {
        "echo $?",
        {"echo", "42", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


/* Take positional arguments from shell.argv */
static void
test_param_expansion_digits() {
    TestCase c = {
        "echo $1 $2 foo$42bar",
        {"echo", "test_arg", "foobar", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_dollar_sign_as_literal() {
    TestCase c = {
        "echo $ $- foo$ 42$",
        {"echo", "$", "$-", "foo$", "42$", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_undeclared_var() {
    unsetenv("UNDECLARED_TEST_VAR");

    TestCase c = {
        "echo $UNDECLARED_TEST_VAR",
        {"echo", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_with_braces() {
    TestCase c = {
        "echo ${TEST_ENV_VAR} ${1} ${} \\${2}",
        {"echo", "test", "test_arg", "${2}", NULL},
    };

    tokenize(c.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c.expected_args);

    free_tokens(ta);
}


static void
test_param_expansion_with_glued_tokens() {
    setenv("U", "", 1);
    setenv("S", " ", 1);
    setenv("B", " a", 1);
    setenv("E", "a ", 1);

    TestCase c1 = {
        "echo $TEST_ENV_VAR\"$TEST_ENV_VAR\"'$TEST_ENV_VAR' a''b",
        {"echo", "testtest$TEST_ENV_VAR", "ab", NULL},
    };
    TestCase c2 = {
        "/'y'$U'z'/ /'y'$S'z'/ /'y'$B/ /$E'z'/",
        {"/yz/", "/y", "z/", "/y", "a/", "/a", "z/", NULL},
    };

    tokenize(c1.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c1.expected_args);

    free_tokens(ta);

    tokenize(c2.command, &ta);
    perform_expansions(&ta, &msg);

    tokens_to_str_arr(ta.tokens, arr);

    ASSERT_EQ(arr, c2.expected_args);

    free_tokens(ta);

    unsetenv("U");
    unsetenv("S");
    unsetenv("B");
    unsetenv("E");
}


int
main() {
    set_up();
    RUN_TESTS(
        "expansion",
        test_param_expansion_with_home_var,
        test_param_expansion_unquoted,
        test_param_expansion_double_quoted,
        test_param_expansion_single_quoted,
        test_param_expansion_with_slash,
        test_param_expansion_exit_code,
        test_param_expansion_digits,
        test_param_expansion_dollar_sign_as_literal,
        test_param_expansion_undeclared_var,
        test_param_expansion_with_braces,
        test_param_expansion_with_glued_tokens,
    );
}
