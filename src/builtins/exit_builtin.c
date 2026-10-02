#include <stdlib.h>

#include "builtins/exit_builtin.h"
#include "util.h"
#include "shell.h"


int exit_builtin(int argc, char** argv) {
    if (argc > 1) {
        char *endptr;
        long val = strtol(argv[1], &endptr, 10);

        if (*endptr == '\0') {
            shell.exit_code = (int)(val % 256);
            if (shell.exit_code < 0) shell.exit_code += 256;
        } else {
            print_err(argv[0], "The exit code must be a number");
            shell.exit_code = 2;
            exit(shell.exit_code);
        }
    }

    if (argc > 2) {
        print_err("exit", "too many arguments");
        return EXIT_FAILURE;
    }

    exit(shell.exit_code);
}
