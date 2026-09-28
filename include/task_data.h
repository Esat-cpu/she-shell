#ifndef TASK_DATA_H
#define TASK_DATA_H

#include <stddef.h>
#include <stdbool.h>


typedef struct {
    int (*pipefd)[2];
    size_t pos;
    size_t count;
} Pipe;


typedef struct TaskData {
    bool fg;
    bool exec_builtins;
    bool apply_redir;
    bool apply_pipe;
    Pipe pipe;
} TaskData;

#endif
