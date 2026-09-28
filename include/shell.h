#ifndef SHELL_H
#define SHELL_H

#include <signal.h>
#include <limits.h>
#include <stdbool.h>


struct ShellState {
    // Shell's name, taken from argv[0]
    const char* name;

    // Exit code of the last command
    volatile sig_atomic_t exit_code;

    // Working directories
    char cwd[PATH_MAX];
    char oldpwd[PATH_MAX];

    // Arguments for context
    /* When executing a script, set them according to script's arguments. */
    int argc;
    char** argv;

    // User info
    const char* home;
    const char* user;

    // Shell interactive mode
    bool interactive;
    // If true, the shell exits on certain command failures
    bool errexit;

    // Terminal fd
    int terminal;
    // Shell's process group
    pid_t pgid;
};


extern struct ShellState shell;

#endif
