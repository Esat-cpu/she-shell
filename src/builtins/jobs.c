#include "builtins/jobs.h"
#include "job_control.h"


int jobs(int argc, char** argv) {
    (void)argc;
    (void)argv;
    manage_bg_jobs();

    if (bg_jobs.len == 0)
        return 0;

    for_each_bg_job(j) {
        if (j->status == RUNNING || j->status == STOPPED)
            print_job_message(*j, 0);
    }

    return 0;
}
