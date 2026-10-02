#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/wait.h>

#include "job_control.h"
#include "task_data.h"
#include "util.h"
#include "ast.h"
#include "shell.h"


typedef struct Process {
    pid_t    pid;
    RunState state;
    int      status;

    struct Process *next;
} Process;


int create_process(Job* job, int (*f)(Node*, TaskData),
                            Node* node, TaskData data) {
    pid_t pid = fork();

    if (pid == 0) {
        // Return signals to their default behavior
        signal(SIGINT,  SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        // Add this process to job's process group.
        // This is also done in the parent to avoid a race where the process
        // receives a signal before the parent can add it to the group.
        setpgid(0, job->pgid);

        _exit(f(node, data));
    }
    else if (pid > 0) {
        // If this is the first process in the process group, make it leader
        if (job->pgid == 0)
            job->pgid = pid;

        // Add this process to group
        setpgid(pid, job->pgid);

        // If fg is set, make the group foreground group
        if (data.fg && shell.interactive)
            tcsetpgrp(shell.terminal, job->pgid);

        Process* p = scalloc(sizeof(Process));
        p->pid   = pid;
        p->state = RUNNING;
        p->next  = job->pipeline;
        job->pipeline = p;
        job->status   = RUNNING;
    }
    else {
        print_err("fork", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


int mark_status(Job* job, pid_t pid, int status) {
    if (pid > 0) {
        for_each_process(p, job) {
            if (p->pid == pid) {
                job->changed = true;
                p->status = status;

                if (WIFSTOPPED(status)) {
                    p->state    = STOPPED;
                    job->status = STOPPED;
                }
                else
                    p->state = COMPLETED;

                return 0;
            }
        }

        return -1;
    }

    else if (pid == 0 || errno == ECHILD) {
        return -1;
    }
    else {
        print_err("waitpid", strerror(errno));
        return -1;
    }
}


void wait_for_job_blocking(Job* job) {
    int status;
    pid_t pid;

    do
        pid = waitpid(-job->pgid, &status, WUNTRACED);
    while(!mark_status(job, pid, status) && job->status != STOPPED);

    Process* p = job->pipeline;

    if (WIFSIGNALED(p->status)) {
        shell.exit_code = 128 + WTERMSIG(p->status);
        free_processes(job);
    }

    else if (WIFSTOPPED(p->status)) {
        job->status = STOPPED;
        add_job_to_bg_jobs(job);
        shell.exit_code = 128 + WSTOPSIG(p->status);
    }

    else {
        shell.exit_code = WEXITSTATUS(p->status);
        free_processes(job);
    }
}


void free_processes(Job* job) {
    Process* p = job->pipeline;
    while (p) {
        Process *tmp = p;
        p = p->next;
        free(tmp);
    }
    job->pipeline = NULL;
}


/* Background Jobs */

BgJobs bg_jobs;


int add_bg_job(int (*f)(Node*, TaskData), Node* node, TaskData data) {
    Job job = {
        .pipeline = NULL,
        .pgid = 0,
        .status = RUNNING,
    };

    int c = create_process(&job, f, node, data);
    if (c) return c;

    add_job_to_bg_jobs(&job);
    printf("[%d]\t%d\n", job.job_id, job.pgid);

    return 0;
}


void add_job_to_bg_jobs(Job *job) {
    job->job_id = bg_jobs.len + 1;
    bg_jobs.jobs[bg_jobs.len++] = *job;
}


static void update_job_state(Job* job) {
    bool all_completed  = true;
    bool all_stopped    = true;

    for_each_process(p, job) {
        if (p->state != COMPLETED) {
            all_completed = false;

            if (p->state != STOPPED)
                all_stopped = false;
        }
    }

    if (all_completed)
        job->status = COMPLETED;
    else if (all_stopped)
        job->status = STOPPED;
    else
        job->status = RUNNING;
}


// Notify background job state changes and free completed job processes.
void manage_bg_jobs(void) {
    int status;
    pid_t pid;

    for_each_bg_job(j) {
        do
            pid = waitpid(-j->pgid, &status, WUNTRACED|WNOHANG);
        while (!mark_status(j, pid, status));
    }

    // Completed jobs are not removed from the array, so the last active job
    // ID determines the new array length.
    size_t last_alive = 0;
    for_each_bg_job(j) {
        if (j->changed) {
            j->changed = false;

            update_job_state(j);

            if (j->status == COMPLETED && !j->notified) {
                printf("[%d]\t%d\tcompleted\n", j->job_id, j->pgid);
                free_processes(j);
                j->notified = true;
            }
            else if (j->status == STOPPED) {
                if (!j->notified)
                    printf("[%d]\t%d\tstopped\n", j->job_id, j->pgid);

                j->notified = true;
            }
        }

        if (j->status != COMPLETED)
            last_alive = j->job_id;
    }

    bg_jobs.len = last_alive;
}


void free_bg_job_processes(void) {
    for_each_bg_job(j) {
        free_processes(j);
    }
}
