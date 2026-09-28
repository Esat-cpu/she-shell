#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "tokenize.h"
#include "token.h"
#include "str_util.h"

#define MAX_BUF 4096


// Fills a allocated array with only strings of tokens
void tokens_to_str_arr(Token* tokens, char** arr) {
    int i;
    for (i = 0; tokens[i].value; ++i)
        arr[i] = tokens[i].value;
    arr[i] = NULL;
}


// tokenize
void tokenize(const char* input, TokenArray* t) {
    QuoteType status = NORMAL;
    TokenType type   = T_WORD;
    bool escape = false;
    bool space  = true;

    String str = new_string();
    *t = new_token_array();

    for (const char* ch = input; *ch; ch++) {
        //  If escape status is 1 and the status is NORMAL,
        //+ append the current character as-is and continue.
        //  If the status is DOUBLE_Q,
        //+ escape only special characters in double quotes.
        //+ With other characters take the '\' character as well.
        //  Also if the current character is space,
        //+ the space flag will not be changed.
        if (escape) {
            const char* special_characters = "$\"\\";
            if (status == DOUBLE_Q && !strchr(special_characters, *ch))
                add_chr_to_str(&str, '\\');

            add_chr_to_str(&str, *ch);
            escape = false;
            continue;
        }

        //  Check if character is '\', escape status is 0 and
        //+ it isn't enclosed in single quotes
        //  if so, do not take the '\' character and set
        //+ escape status to 1
        if (*ch == '\\' && !escape && status != SINGLE_Q) {
            // If the character after escape character is '$',
            // switch escape to special character '\x01' for the
            // parameter expansion.
            if (*(ch+1) == '$') {
                add_chr_to_str(&str, '\x01');
                continue;
            }

            escape = true;
            continue;
        }

        // Semicolon case
        if (*ch == ';' && status == NORMAL) {
            if (add_token(t, str.data, NORMAL, type))
                clear_str(&str);

            add_token(t, ";", NORMAL, T_SEMI);
            continue;
        }

        // Operator cases
        // PIPE
        if (*ch == '|' && status == NORMAL) {
            const char *next_ch = ch+1;

            if (add_token(t, str.data, NORMAL, type))
                clear_str(&str);

            // PIPE Operator (|)
            if (*next_ch != '|') {
                add_token(t, "|", NORMAL, T_PIPE);
                continue;
            }
            // OR Operator (||)
            else {
                add_token(t, "||", NORMAL, T_OR);
                ch++;
                continue;
            }
        }

        // AND
        if (*ch == '&' && status == NORMAL) {
            const char *next_ch = ch+1;

            // AND Operator (&&)
            if (*next_ch == '&') {
                if (add_token(t, str.data, NORMAL, type))
                    clear_str(&str);

                add_token(t, "&&", NORMAL, T_AND);
                ch++;
                continue;
            }
            // Background operator (&)
            else
                add_token(t, "&", NORMAL, T_AMPER);

            continue;
        }

        // Redirection out
        if (*ch == '>' && status == NORMAL) {
            const char *next_ch = ch+1;

            if (add_token(t, str.data, NORMAL, type))
                clear_str(&str);

            // >> operator
            if (*next_ch == '>') {
                add_token(t, ">>", NORMAL, T_REDIR_OUT_APPEND);
                ch++;
            }
            // > operator
            else
                add_token(t, ">", NORMAL, T_REDIR_OUT);

            continue;
        }

        // Redirection out for fileno 2 (stderr)
        // It requires a space before it
        if (*ch == '2' && space && status == NORMAL) {
            const char *next_ch = ch+1;

            // If it is a redirection operator
            if (*next_ch == '>') {
                if (add_token(t, str.data, NORMAL, type))
                    clear_str(&str);

                const char *next_next_ch = NULL;
                if (*next_ch) next_next_ch = next_ch+1;

                // 2>> operator
                if (next_next_ch && *next_next_ch == '>') {
                    add_token(t, "2>>", NORMAL, T_REDIR_ERR_OUT_APPEND);
                    ch += 2;
                }
                // 2> operator
                else {
                    add_token(t, "2>", NORMAL, T_REDIR_ERR_OUT);
                    ch++;
                }

                continue;
            }
        }

        // Redirection in
        // < operator
        if (*ch == '<' && status == NORMAL) {
            if (add_token(t, str.data, NORMAL, type))
                clear_str(&str);

            add_token(t, "<", NORMAL, T_REDIR_IN);
            continue;
        }

        // comment case
        if (*ch == '#' && space) {
            t->tokens[t->len].value = NULL;
            free(str.data);
            return;
        }

        // space case in normal mode
        if (*ch == ' ' && status == NORMAL) {
            if (add_token(t, str.data, NORMAL, type))
                clear_str(&str);

            space = true;
            continue;
        }
        space = false;


        // double quote case
        if (*ch == '"') {
            if (status == NORMAL) {
                if (add_token(t, str.data, NORMAL, type))
                    clear_str(&str);

                status = DOUBLE_Q;
                continue;
            }

            if (status == DOUBLE_Q) {
                if (add_token(t, str.data, DOUBLE_Q, type))
                    clear_str(&str);

                status = NORMAL;
                continue;
            }
        }

        // single quote case
        if (*ch == '\'') {
            if (status == NORMAL) {
                if (add_token(t, str.data, NORMAL, type))
                    clear_str(&str);

                status = SINGLE_Q;
                continue;
            }

            if (status == SINGLE_Q) {
                if (add_token(t, str.data, SINGLE_Q, type))
                    clear_str(&str);

                status = NORMAL;
                continue;
            }
        }

        // normal character case
        add_chr_to_str(&str, *ch);
    }

    add_token(t, str.data, NORMAL, type);
    t->tokens[t->len].value = NULL;
    free(str.data);
}
