#include <stdlib.h>
#include <signal.h>

#include "builtins/fg.h"
#include "job_control.h"
#include "util.h"
#include "str_util.h"
#include "shell.h"


static void no_such_job_err(const char *argv0, const char *buf) {
    String str = new_string();
    add_slice_to_str(&str, "No such job: ");
    add_slice_to_str(&str, buf);
    print_err(argv0, str.data);
    free(str.data);
}


int fg(int argc, char** argv) {
    (void)argc;
    manage_bg_jobs();

    if (bg_jobs.len == 0) {
        print_err(argv[0], "you have no background jobs");
        return EXIT_FAILURE;
    }

    Job* job = NULL;

    // Parse %N
    if (argc == 2) {
        char* buf = argv[1];

        if (buf[0] != '%') {
            no_such_job_err(argv[0], buf);
            return EXIT_FAILURE;
        }

        char *endptr;
        // Array indices start at 0, while job IDs start at 1 (%N)
        int ind = strtol(&buf[1], &endptr, 10) - 1;

        if (
                buf[1] == '\0'
             || *endptr != '\0'
             || ind < 0 || ind >= bg_jobs.len
        ) {
            no_such_job_err(argv[0], buf);
            return EXIT_FAILURE;
        }

        job = &bg_jobs.jobs[ind];

        if (job->status != STOPPED && job->status != RUNNING) {
            no_such_job_err(argv[0], buf);
            return EXIT_FAILURE;
        }
    }

    else if (argc > 2) {
        print_err(argv[0], "too many arguments");
        return EXIT_FAILURE;
    }

    // With no args, use the last alive job in the bg array
    else {
        for_each_bg_job(j) {
            if (j->status == STOPPED || j->status == RUNNING) job = j;
        }

        if (!job) {
            print_err(argv[0], "No job found");
            return EXIT_FAILURE;
        }
    }

    Job fg_job = {
        .pgid     = job->pgid,
        .pipeline = job->pipeline,
        .command  = job->command,
        .job_id   = job->job_id,
        .status   = RUNNING,
    };

    // Remove job from bg jobs by marking it COMPLETED and notified
    job->notified = true;
    job->status = COMPLETED;
    job->command = NULL;
    job->pipeline = NULL;

    print_job_message(fg_job, 1);

    if (shell.interactive)
        tcsetpgrp(shell.terminal, fg_job.pgid);

    kill(-job->pgid, SIGCONT);

    wait_for_job_blocking(&fg_job);

    if (shell.interactive)
        tcsetpgrp(shell.terminal, shell.pgid);

    return shell.exit_code;
}
