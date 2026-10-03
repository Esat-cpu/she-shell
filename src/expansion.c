#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>

#include "expansion.h"
#include "str_util.h"
#include "token.h"
#include "util.h"
#include "shell.h"


// Traverse the token array and apply word splitting to unquoted tokens:
// - Split words on spaces
// - Remove empty unquoted words
static void split_normal_words(TokenArray* ta) {
    TokenArray ta_new = new_token_array();

    for_each_token (t, ta) {
        if (t->quote_type == NORMAL) {
            if (t->value[0] == '\0')
                continue;

            char *c = strtok(t->value, " \t");
            if (!c) continue;

            add_token(&ta_new, c, NORMAL, t->token_type, t->glued);

            while ((c = strtok(NULL, " \t")))
                add_token(&ta_new, c, NORMAL, t->token_type, t->glued);
        }
        else
            add_token(
                    &ta_new, t->value, t->quote_type, t->token_type, t->glued);
    }

    free_tokens(*ta);
    *ta = ta_new;
}


// Finds dollar signs and the special escape character that is
// set in tokenizer.
static char* next_dollar_sign(char* buf) {
    while (*buf != '\0') {
        if (*buf == '\x01' || *buf == '$')
            return buf;

        buf++;
    }

    return NULL;
}


static char* expand_param_in_one_expr(char* start, String* str) {
    char* ch   = start;

    // $? case
    // Expand parameter as exit code
    if (*ch == '?') {
        add_int_to_str(str, shell.exit_code);
        return ch+1;
    }

    // $0, $1, $2 ... case
    // Expand parameter as context argument
    else if (isdigit((unsigned char) *ch)) {
        int ind = strtol(start, &ch, 10);

        if (ind < shell.argc)
            add_slice_to_str(str, shell.argv[ind]);
    }

    // $ENV_VARIABLE case
    // Expand parameter as environment variable
    else if (isalnum((unsigned char) *ch) || *ch == '_') {
        while ((isalnum((unsigned char) *ch) || *ch == '_'))
            ch++;

        char* env_name = strndup(start, (ch - start));

        char* env_value;
        if ((env_value = getenv(env_name)))
            add_slice_to_str(str, env_value);

        free(env_name);
    }

    // $$ case
    // Expand parameter as shell's PID
    else if (*ch == '$') {
        add_int_to_str(str, (int)getpid());
        ch++;
    }

    return ch;
}


static int expand_param_in_token(Token* token, char **error_out) {
    // New buffer for expanded token value
    String str = new_string();

    char *ch    = token->value;
    char *start = token->value;

    while ((ch = next_dollar_sign(ch))) {
        add_span_to_str(&str, start, ch);

        // Escape case
        if (*ch == '\x01') {
            ch++;
            add_chr_to_str(&str, *ch);
            ch++;
        }

        else { /* *ch == '$' */
            ch++;
            char* pos;

            // ${PARAM} case
            if (*ch == '{') {
                ch++;
                pos = expand_param_in_one_expr(ch, &str);

                // Abort expansion on unclosed '{' so the whole line
                // is skipped.
                if (*pos != '}') {
                    free(str.data);
                    *error_out = sstrdup("Parse error, expected '}'");
                    return -1;
                }

                ch = pos + 1;
            }

            // $PARAM case
            else {
                pos = expand_param_in_one_expr(ch, &str);

                // If no case matches, take the dollar sign as-is
                if (pos == ch)
                    add_chr_to_str(&str, '$');
                else
                    ch = pos;
            }
        }

        start = ch;
    }

    add_slice_to_str(&str, start);

    free(token->value);
    token->value = str.data;
    return 0;
}


/* Glob Expansion */
static void expand_glob_in_token(Token* token) {
    glob_t globbuf;
    String str = new_string();

    glob(token->value, GLOB_NOCHECK, NULL, &globbuf);

    for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
        add_chr_to_str(&str, ' ');
        add_slice_to_str(&str, globbuf.gl_pathv[i]);
    }

    globfree(&globbuf);
    free(token->value);
    token->value = str.data;
}


/* Tilde Expansion */
static void expand_tilde(TokenArray* ta) {
    if (!shell.home) return;

    // Expand `~` to the user's home directory when it is the first character
    // of a NORMAL quote type token. A `~` followed by an alphanumeric
    // character is not expanded.
    for_each_token (t, ta) {
        if (t->quote_type == NORMAL && t->value[0] == '~'
                && (!t->value[1] || !isalnum((unsigned char)t->value[1])))
        {
            String str = new_string();
            add_slice_to_str(&str, (char*)shell.home);
            add_slice_to_str(&str, &t->value[1]);
            free(t->value);
            t->value = str.data;
        }
    }
}


int perform_expansions(TokenArray* ta, char **error_out) {
    expand_tilde(ta);

    for_each_token (token, ta) {
        if (strchr(token->value, '$') && token->quote_type != SINGLE_Q) {
            int e = expand_param_in_token(token, error_out);
            if (e) return e;
        }

        if (token->quote_type == NORMAL)
            expand_glob_in_token(token);
    }

    split_normal_words(ta);

    glue_tokens(ta);

    return 0;
}
