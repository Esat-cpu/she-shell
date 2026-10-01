#ifndef TASK_DATA_H
#define TASK_DATA_H

#include <stddef.h>
#include <stdbool.h>


typedef struct {
    int (*pipefd)[2];

    // Position of the current command in the pipeline.
    size_t pos;

    size_t count;
} Pipe;


typedef struct TaskData {
    // `fg` must be initialized to true and only changed by execute_ast().
    bool fg;
    bool exec_builtins;
    bool apply_redir;
    bool apply_pipe;
    Pipe pipe;
} TaskData;

#endif
