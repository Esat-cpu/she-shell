#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "builtins/export.h"
#include "str_util.h"
#include "util.h"


static int is_identifier(const char* s, const char *end) {
    if (!isalpha((unsigned char) *s) && *s != '_')
        return 0;

    for (const char *i = s+1; *i && i != end; ++i) {
        if (!isalnum((unsigned char) *i) && *i != '_')
            return 0;
    }

    return 1;
}


int export(int argc, char** argv) {
    if (argc == 1) {
        extern char** environ;

        for (char** env = environ; *env; ++env) {
            printf("%s\n", *env);
        }

        return 0;
    }

    int status = 0;

    for (int i = 1; i < argc; ++i) {
        char* eq = strchr(argv[i], '=');

        if (!eq) {
            print_err(argv[0], "Expected NAME=value");
            status = 1;
            continue;
        }

        if (!is_identifier(argv[i], eq)) {
            print_err(argv[0], "Not an identifier");
            status = 1;
            continue;
        }

        String str = new_string();
        add_span_to_str(&str, argv[i], eq);

        setenv(str.data, eq+1, 1);

        free(str.data);
    }

    return status;
}
