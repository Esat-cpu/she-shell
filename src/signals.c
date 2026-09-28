#include <signal.h>
#include <sys/wait.h>
#include <limits.h>
#include <stdio.h> // IWYU pragma: keep
#include <readline/readline.h>

#include "signals.h"
#include "job_control.h"
#include "prompt_build.h"
#include "shell.h"


volatile sig_atomic_t in_readline = false;


static void update_job_state(Job* job) {
    bool all_exited  = true;
    bool all_stopped = true;

    for_each_process(p, job) {
        if (p->state != EXITED) {
            all_exited  = false;

            if (p->state != STOPPED)
                all_stopped = false;
        }
    }

    if (all_exited)
        job->status = EXITED;
    else if (all_stopped)
        job->status = STOPPED;
    else
        job->status = RUNNING;
}


void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;

    for_each_bg_job(j) {
        do
            pid = waitpid(-j->pgid, &status, WUNTRACED|WNOHANG);
        while (!mark_status(j, pid, status));

        update_job_state(j);
    }
}


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
