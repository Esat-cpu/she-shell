#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <stdbool.h>


struct ArgFlags {
    bool no_profile;
    bool should_exit;
    bool login_shell;
    char* command;
    const char* script;
    int script_index;
};

extern struct ArgFlags args;

void parse_arguments(int argc, char **argv);

#endif
