#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <pwd.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "prompt_build.h"
#include "argument_parser.h"
#include "util.h"
#include "executor.h"
#include "job_control.h"
#include "signals.h"
#include "shell.h"


// History file, set to NULL if saving history file is not required.
// It will be written to and read from the user's home directory.
static const char* HIS_FILE = ".she_history";
// The file that will be executed on startup after setup and parse_arguments
// functions if --no-profile flag is not set.
static const char* RC_FILE = ".sherc";
// Read in login shells (-l, or argv[0] starting with '-'), unless
// --no-profile is set.
static const char* PROFILE = ".she_profile";

static char* command = NULL;


static void clean_exit(void) {
    free(command);

    free_bg_job_processes();

    if (shell.interactive) {
        if (shell.terminal != 0)
            close(shell.terminal);

        char history_file[PATH_MAX];

        if (shell.home && HIS_FILE) {
            snprintf(history_file, sizeof(history_file),
                    "%s/%s", shell.home, HIS_FILE);
            write_history(history_file);
        }
    }
}


// Set shell's name from argv[0]
static void set_shell_name(const char *argv0) {
    const char *s = strrchr(argv0, '/');
    shell.name = s ? (s + 1) : argv0;
}


// This will be executed on shell's startup
static void setup(int argc, char** argv) {
    atexit(clean_exit);
    set_shell_name(argv[0]);
    shell.argc = argc;
    shell.argv = argv;

    struct passwd* pw = getpwuid(getuid());

    if (!(shell.home = getenv("HOME")))
        shell.home = pw->pw_dir;

    shell.user = pw->pw_name;

    if (isatty(STDIN_FILENO)) {
        shell.interactive = true;

        while (tcgetpgrp(shell.terminal) != (shell.pgid = getpgrp()))
            kill(-shell.pgid, SIGTTIN);

        // Handle signals
        signal(SIGINT,  sigint_handler);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        // Terminal fd
        int title_fd = open("/dev/tty", O_RDWR);
        shell.terminal = (title_fd < 0) ? STDIN_FILENO : title_fd;

        // Shell is the process group leader
        // This group is terminal's foreground process group
        if (tcgetpgrp(shell.terminal) != shell.pgid) {
            setpgid(0, 0);
            shell.pgid = getpgrp();
            tcsetpgrp(shell.terminal, shell.pgid);
        }
    }
    else
        shell.interactive = false;

    // Set working directories
    if (getcwd(shell.cwd, sizeof(shell.cwd)) == NULL) {
        perror("getcwd");
        exit(errno);
    }
    strcpy(shell.oldpwd, shell.cwd);

    // Read history from HIS_FILE
    if (shell.interactive && shell.home && HIS_FILE) {
        char history_file[PATH_MAX];
        snprintf(history_file, sizeof(history_file),
                "%s/%s", shell.home, HIS_FILE);
        read_history(history_file);
    }

    // Set environment variables if not set
    setenv("HOME", shell.home, 0);
    setenv("USER", shell.user, 0);
    setenv("SHELL", pw->pw_shell, 0);
}


int main(int argc, char** argv) {
    setup(argc, argv);
    parse_arguments(argc, argv);

    // Execute profile file if it exists
    if (args.login_shell && PROFILE && shell.home && !args.no_profile) {
        char pro_path[PATH_MAX];
        snprintf(pro_path, sizeof(pro_path), "%s/%s", shell.home, PROFILE);

        if (access(pro_path, F_OK) == 0)
            execute_file(pro_path);
    }

    // Execute startup file if it exists
    if (RC_FILE && shell.interactive && shell.home && !args.no_profile) {
        char rc_path[PATH_MAX];
        snprintf(rc_path, sizeof(rc_path), "%s/%s", shell.home, RC_FILE);

        if (access(rc_path, F_OK) == 0)
            execute_file(rc_path);
    }

    // Execute the command that is given as an argument
    if (args.command) {
        char *error_message = NULL;
        ExeResult e = execute_line(args.command, &error_message);

        if (e) {
            print_err(error_message, NULL);
            free(error_message);
        }
    }

    // Execute the script file that is given as an argument
    if (args.script) {
        shell.argv += args.script_index;
        shell.argc -= args.script_index;

        execute_file(args.script);

        shell.argv = argv;
        shell.argc = argc;
    }

    // This condition determined by argument parser according to arguments
    if (args.should_exit)
        exit(shell.exit_code);

    size_t size = 0;

    while (1) {
        shell.home = getenv("HOME");

        if (shell.interactive) {
            char prompt[PATH_MAX];
            prompt_build(prompt, PATH_MAX);

            // Set/Reset terminal title before prompt.
            if (shell.terminal != 0)
                dprintf(shell.terminal, "\033]0;she\007");

            free(command);

            in_readline = 1;
            command = readline(prompt);
            in_readline = 0;

            if (!command)
                exit(shell.exit_code);

            // Set terminal title to running command.
            if (shell.terminal != 0)
                dprintf(shell.terminal, "\033]0;%s\007", command);

            add_history(command);
        }
        else {
            ssize_t len = getline(&command, &size, stdin);

            if (len == -1) {
                if (feof(stdin)) break;

                perror("getline");
                exit(EXIT_FAILURE);
            }
        }

        char *error_message = NULL;
        ExeResult e = execute_line(command, &error_message);

        if (e) {
            print_err(error_message, NULL);
            free(error_message);
        }

        manage_bg_jobs();

        if (shell.errexit && shell.exit_code != 0)
            exit(shell.exit_code);
    }

    return 0;
}
