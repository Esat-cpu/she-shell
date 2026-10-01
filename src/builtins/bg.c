#include <signal.h>
#include <stdio.h>

#include "job_control.h"
#include "util.h"


int bg(int argc, char** argv) {
    (void)argc;
    manage_bg_jobs();

    if (bg_jobs.len == 0) {
        print_err(argv[0], "you have no background jobs");
        return 1;
    }

    Job* job = &bg_jobs.jobs[bg_jobs.len-1];

    printf("[%d]\t%d\tcontinued\n", job->job_id, job->pgid);

    job->notified = false;
    job->status = RUNNING;
    kill(-job->pgid, SIGCONT);

    return 0;
}
