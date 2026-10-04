#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "builtins/exec.h"
#include "signals.h"
#include "util.h"


int exec(int argc, char** argv) {
    if (argc == 1)
        return 0; /* `exec > file` (persistent redirection) not supported */

    signal(SIGINT,  SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);

    execvp(argv[1], &argv[1]);
    int err = errno;

    signal(SIGINT,  sigint_handler);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    print_err(argv[1], strerror(err));

    return err == ENOENT ? 127 : 126;
}
