#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "argument_parser.h"
#include "shell.h"


struct ArgFlags args = {
    .no_profile   = false,
    .should_exit  = false,
    .fast_exit    = false,
    .login_shell  = false,
    .command      = NULL,
    .script       = NULL,
    .script_index = 0,
};


static void print_usage(FILE* out_stream) {
    fprintf(out_stream,
            "Usage: %s [OPTIONS] [SCRIPT FILE [ARG1,ARG2,...]]\n", shell.name);

    fprintf(out_stream, "Options:\n"
            "\t-h, --help       Show this helper message\n"
            "\t-v, --version    Show version info\n"
            "\t-c <COMMAND>     Execute the given command line\n"
            "\t--no-profile     Do not execute the startup files\n"
            "\t-l, --login      Login shell\n"
    );
}


static void print_version() {
    printf("%s %s %s\n", shell.name, SHE_VERSION, SHE_BUILD);
}


void parse_arguments(int argc, char **argv) {
    if (argc <= 1) return;
    int opt;

    // Convert long options to their short equivalents.
    // Every long option must have a corresponding short option.
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            strcpy(argv[i], "-h");
        }
        else if (strcmp(argv[i], "--version") == 0) {
            strcpy(argv[i], "-v");
        }
        else if (strcmp(argv[i], "--no-profile") == 0) {
            strcpy(argv[i], "-n");
        }
        else if (strcmp(argv[i], "--login") == 0) {
            strcpy(argv[i], "-l");
        }
    }

    while ((opt = getopt(argc, argv, "hvc:nl")) != -1) {
        switch (opt) {
            case 'h':
                print_usage(stdout);
                args.fast_exit = true;
                return;
            case 'v':
                print_version();
                args.fast_exit = true;
                return;
            case 'c':
                args.command = optarg;
                args.should_exit = true;
                return;
            case 'n':
                args.no_profile = true;
                break;
            case 'l':
                args.login_shell = true;
                break;
            default: /* '?' */
                print_usage(stderr);
                shell.exit_code = EXIT_FAILURE;
                args.fast_exit = true;
                return;
        }
    }

    if (argv[0][0] == '-')
        args.login_shell = true;

    if (optind < argc) {
        args.script = argv[optind];
        args.script_index = optind;
        args.should_exit = true;
    }
}
