#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "prompt_build.h"
#include "shell.h"

#define IG_S "\001" // RL_PROMPT_START_IGNORE
#define IG_E "\002" // RL_PROMPT_END_IGNORE

#define GREEN IG_S "\033[1;32m" IG_E
#define BLUE  IG_S "\033[1;34m" IG_E
#define RED   IG_S "\033[1;31m" IG_E
#define RESET IG_S "\033[0m" IG_E

#define SIG_ENTRY(s) { s, &#s[3] }


// Showing the error code in the prompt if it is not 0
static const char fmt[]   = GREEN "%s " BLUE "%s" RESET "> ";
static const char fmt_s[] = GREEN "%s " BLUE "%s " RED "[%s]" RESET "> ";
static const char fmt_d[] = GREEN "%s " BLUE "%s " RED "[%d]" RESET "> ";

static const struct { int num; const char* name; } sig_table[] = {
    SIG_ENTRY(SIGINT),  SIG_ENTRY(SIGTSTP), SIG_ENTRY(SIGKILL),
    SIG_ENTRY(SIGHUP),  SIG_ENTRY(SIGQUIT), SIG_ENTRY(SIGILL),
    SIG_ENTRY(SIGABRT), SIG_ENTRY(SIGFPE),  SIG_ENTRY(SIGSEGV),
    SIG_ENTRY(SIGPIPE), SIG_ENTRY(SIGALRM), SIG_ENTRY(SIGTERM),
    SIG_ENTRY(SIGSTOP), SIG_ENTRY(SIGTTIN), SIG_ENTRY(SIGTTOU),
};


static const char* signal_name_from_exit_code(int code) {
    if (code <= 128) return NULL;

    for (size_t i = 0; i < sizeof sig_table / sizeof *sig_table; ++i) {
        if (sig_table[i].num == code - 128)
            return sig_table[i].name;
    }

    return NULL;
}


void prompt_build(char* prompt, size_t prompt_size) {
    // The path that will appear in the prompt
    char prmpt_cwd[PATH_MAX];

    // '~' contraction for prompt
    size_t home_len = shell.home ? strlen(shell.home) : 0;

    if (home_len > 1
            && strncmp(shell.cwd, shell.home, home_len) == 0
            && (shell.cwd[home_len] == '\0' || shell.cwd[home_len] == '/'))
        snprintf(prmpt_cwd, PATH_MAX, "~%s", shell.cwd + home_len);
    else
        strcpy(prmpt_cwd, shell.cwd);

    const char* sig_name = signal_name_from_exit_code(shell.exit_code);

    if (shell.exit_code == 0)
        snprintf(prompt, prompt_size, fmt, shell.user, prmpt_cwd);

    else if (sig_name)
        snprintf(prompt, prompt_size, fmt_s, shell.user, prmpt_cwd, sig_name);

    else
        snprintf(prompt, prompt_size, fmt_d,
                        shell.user, prmpt_cwd, shell.exit_code);
}
