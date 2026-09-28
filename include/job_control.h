#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <unistd.h>
#include <stdbool.h>

#include "task_data.h"
#include "ast.h"

#define for_each_process(p, job) \
    for (Process* (p) = (job)->pipeline; (p) != NULL; (p) = (p)->next)

#define for_each_bg_job(j) \
    for (Job* (j) = &bg_jobs.jobs[0]; (j) < bg_jobs.jobs + bg_jobs.len; ++(j))


typedef enum {
    RUNNING=1,
    STOPPED,
    EXITED,
} RunState;


typedef struct Process {
    pid_t    pid;
    RunState state;
    int      status;

    struct Process *next;
} Process;


typedef struct Job {
    Process*  pipeline;
    pid_t     pgid;
    RunState  status;
    int       job_id;
    bool      notified;
} Job;


typedef struct {
    Job* jobs;
    int  len;
    int  cap;
} BgJobs;


int create_process(Job* job, int (*f)(Node*, TaskData),
                            Node* node, TaskData data);

int mark_status(Job* job, pid_t pid, int status);

void start_bg_job_array(void);

int add_bg_job(int (*f)(Node*, TaskData), Node* node, TaskData data);

void add_job_to_bg_jobs(Job job);

void manage_bg_jobs(void);

void free_processes(Job* job);

void free_bg_jobs(void);

extern BgJobs bg_jobs;

#endif
