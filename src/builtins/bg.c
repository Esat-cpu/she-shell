#include <signal.h>
#include <stdlib.h>

#include "job_control.h"
#include "util.h"
#include "str_util.h"


static void no_such_job_err(const char *argv0, const char *buf) {
    String str = new_string();
    add_slice_to_str(&str, "No such job: ");
    add_slice_to_str(&str, buf);
    print_err(argv0, str.data);
    free(str.data);
}


int bg(int argc, char** argv) {
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

        if (bg_jobs.jobs[ind].status == STOPPED) {
            job = &bg_jobs.jobs[ind];
        }
        else if (bg_jobs.jobs[ind].status == RUNNING) {
            print_err(argv[0], "Job is already running");
            return EXIT_FAILURE;
        }
        else {
            no_such_job_err(argv[0], buf);
            return EXIT_FAILURE;
        }
    }

    else if (argc > 2) {
        print_err(argv[0], "too many arguments");
        return EXIT_FAILURE;
    }

    // With no args, continue the last stopped job in the bg array
    else {
        for_each_bg_job(j) {
            if (j->status == STOPPED) job = j;
        }

        if (!job) {
            print_err(argv[0], "Job is already running");
            return EXIT_FAILURE;
        }
    }

    job->notified = false;
    job->status = RUNNING;
    kill(-job->pgid, SIGCONT);
    print_job_message(*job, 1);

    return EXIT_SUCCESS;
}
