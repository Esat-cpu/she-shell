#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <stdio.h> // IWYU pragma: keep
#include <readline/readline.h>

#include "signals.h"
#include "prompt_build.h"
#include "shell.h"


volatile sig_atomic_t in_readline   = false;

// Clear input and go to the next line
void sigint_handler(int sig) {
    (void)sig;  // suppress unused warning
    shell.exit_code = 130;
    write(STDOUT_FILENO, "\n", 1);

    char prompt[PATH_MAX];
    prompt_build(prompt, PATH_MAX);

    if (in_readline) {
        rl_replace_line("", 0);
        rl_on_new_line();
        rl_set_prompt(prompt);
        rl_redisplay();
    }
}
