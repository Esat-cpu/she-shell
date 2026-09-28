#include <stdio.h>

#include "builtins/jobs.h"
#include "job_control.h"


int jobs(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (bg_jobs.len == 0)
        return 0;

    for_each_bg_job(j) {
        if (j->status == RUNNING)
            printf("[%d]\t%d\n", j->job_id, j->pgid);
        else if (j->status == STOPPED)
            printf("[%d]\t%d\tstopped\n", j->job_id, j->pgid);
    }

    return 0;
}
