#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

#include "executor.h"
#include "util.h"
#include "tokenize.h"
#include "token.h"
#include "ast.h"
#include "parser.h"
#include "expansion.h"
#include "job_control.h"
#include "builtins/builtins.h"
#include "shell.h"

#define MAX_LINE_SIZE 4096


// Set open flags and target fd according to redir_type.
static void collect_redir_config(RedirList* r, int *flags) {
    /* open flags */
    if (r->redir_type == T_REDIR_OUT || r->redir_type == T_REDIR_ERR_OUT)
        *flags = O_WRONLY | O_CREAT | O_TRUNC;

    else if (r->redir_type == T_REDIR_IN)
        *flags = O_RDONLY;

    else
        *flags = O_WRONLY | O_CREAT | O_APPEND;

    /* targetfd */
    if (r->redir_type == T_REDIR_OUT || r->redir_type == T_REDIR_OUT_APPEND)
        r->targetfd = STDOUT_FILENO;
    else if (r->redir_type == T_REDIR_IN)
        r->targetfd = STDIN_FILENO;
    else
        r->targetfd = STDERR_FILENO;
}


// Applies redirection operators in one node.
static ExeResult apply_redirections(RedirList* r) {
    int flags;

    while (r) {
        collect_redir_config(r, &flags);
        r->savedfd = dup(r->targetfd);

        int fd = open(r->filename, flags, 0644);
        if (fd == -1) {
            print_err(r->filename, strerror(errno));
            return E_FILE_ERROR;
        }

        dup2(fd, r->targetfd);
        close(fd);

        r = r->next;
    }

    return E_SUCCESS;
}


// Restore fd targets that were redirected by 'apply_redirections' function.
static void restore_redirections(RedirList* r) {
    if (!r) return;

    restore_redirections(r->next);

    dup2(r->savedfd, r->targetfd);
    close(r->savedfd);
}


// Call this function only with command (T_WORD) nodes.
// Takes action according to given TaskData and executes cmd node.
// Returns exit code of the builtin command that was executed,
// returns 127 if builtin command or external command is not found,
// returns 126 if an external command's permission is denied,
// and does not return if the external command is executes successfully.
static int worker(Node* node, TaskData data) {
    if (data.apply_pipe) {
        if (data.pipe.pos > 0)
            dup2(data.pipe.pipefd[(data.pipe.pos)-1][0], STDIN_FILENO);
        if (data.pipe.pos < data.pipe.count)
            dup2(data.pipe.pipefd[data.pipe.pos][1], STDOUT_FILENO);

        for (size_t p = 0; p < data.pipe.count; ++p) {
            close(data.pipe.pipefd[p][0]);
            close(data.pipe.pipefd[p][1]);
        }
    }

    if (data.apply_redir) {
        ExeResult ar = apply_redirections(node->cmd.redir_list);
        if (ar == E_FILE_ERROR)
            return EXIT_FAILURE;
    }

    if (data.exec_builtins) {
        // Execute with function if the command matches a builtin
        for (size_t i = 0; builtins[i].func; ++i) {
            if (strcmp(node->cmd.argv[0], builtins[i].cmd_name) == 0) {
                int exit_code = builtins[i].func(
                        node->cmd.argc,
                        node->cmd.argv
                );
                restore_redirections(node->cmd.redir_list);
                return exit_code;
            }
        }
    }

    execvp(node->cmd.argv[0], node->cmd.argv);

    if (data.apply_redir)
        restore_redirections(node->cmd.redir_list);

    if (errno == EACCES) {
        print_err(node->cmd.argv[0], strerror(errno));
        return 126;
    }
    else {
        print_err(node->cmd.argv[0], "Command not found...");
        return 127;
    }
}


// Call this function only with command (T_WORD) nodes.
// Executes builtins in the shell's main process.
// Stores the executed command's exit code in shell.exit_code.
// Always returns.
static void execute_cmd_node(Node* node, TaskData data) {
    ExeResult ar = apply_redirections(node->cmd.redir_list);
    if (ar == E_FILE_ERROR) {
        restore_redirections(node->cmd.redir_list);
        shell.exit_code = EXIT_FAILURE;
        return;
    }

    // Search for builtins and if matches, execute in the main process
    for (size_t i = 0; builtins[i].func; ++i) {
        if (strcmp(node->cmd.argv[0], builtins[i].cmd_name) == 0) {
            shell.exit_code = builtins[i].func(
                    node->cmd.argc,
                    node->cmd.argv
            );
            restore_redirections(node->cmd.redir_list);
            return;
        }
    }

    Job job = {
        .pipeline = NULL,
        .status = RUNNING,
    };

    data.exec_builtins = false;
    data.apply_redir   = false;

    if (data.fg) {
        if (create_process(&job, worker, node, data) == EXIT_SUCCESS)
            wait_for_job_blocking(&job);
    }
    else
        add_bg_job(worker, node, data);

    if (shell.interactive)
        tcsetpgrp(shell.terminal, shell.pgid);

    restore_redirections(node->cmd.redir_list);
}


// Traverse all pipe nodes and execute them concurrently with pipes
// between them.
static int pipe_traversal(Node* node, Job* job, TaskData* data) {
    if (!node || node->type != T_PIPE) return EXIT_SUCCESS;

    Node* arms[2] = {node->operator.left, node->operator.right};

    int result;

    result = pipe_traversal(arms[0], job, data);
    if (result == EXIT_FAILURE) return EXIT_FAILURE;

    result = pipe_traversal(arms[1], job, data);
    if (result == EXIT_FAILURE) return EXIT_FAILURE;

    for (size_t i = 0; i < 2; ++i) {
        node = arms[i];

        if (node && node->type == T_WORD) {
            int a = create_process(job, worker, node, *data);

            if (a == EXIT_FAILURE)
                return EXIT_FAILURE;

            data->pipe.pos++;
        }
    }

    return EXIT_SUCCESS;
}


// Call this on first pipe (T_PIPE) node on AST.
static void pipe_handle(Node* node, TaskData data) {
    size_t pipe_counter = 0;

    Node* tmp = node;
    // Count pipes
    while (tmp->type == T_PIPE) {
        pipe_counter++;
        tmp = tmp->operator.left;
    }

    int pipefd[pipe_counter][2];

    // Initialize pipes
    for (size_t i = 0; i < pipe_counter; ++i) {
        int p = pipe(pipefd[i]);

        if (p == -1) {
            shell.exit_code = EXIT_FAILURE;
            print_err("pipe", strerror(errno));
            return;
        }
    }

    data.pipe.pipefd   = pipefd;
    data.pipe.count    = pipe_counter;
    data.pipe.pos      = 0;

    data.apply_pipe    = true;
    data.apply_redir   = true;
    data.exec_builtins = true;

    Job job = { .pipeline = NULL, .status = RUNNING };

    int e = pipe_traversal(node, &job, &data);

    for (size_t p = 0; p < pipe_counter; ++p) {
        close(pipefd[p][0]);
        close(pipefd[p][1]);
    }

    if (e == EXIT_FAILURE) {
        shell.exit_code = EXIT_FAILURE;

        if (job.pgid)
            kill(-job.pgid, SIGKILL);

        free_processes(&job);

        if (shell.interactive)
            tcsetpgrp(shell.terminal, shell.pgid);

        return;
    }

    if (data.fg)
        wait_for_job_blocking(&job);
    else {
        add_job_to_bg_jobs(&job);
        printf("[%d]\t%d\t%s\n", job.job_id, job.pgid, job.command);
    }

    if (shell.interactive)
        tcsetpgrp(shell.terminal, shell.pgid);
}


// Traverse the AST, apply semantics and execute the commands.
static void execute_ast(Node* root, TaskData data) {
    if (root->type == T_WORD)
        execute_cmd_node(root, data);

    // Execute left side of semicolon, if there are nodes on right,
    // execute them separately.
    else if (root->type == T_SEMI) {
        execute_ast(root->operator.left, data);

        if (shell.errexit && shell.exit_code != 0)
            exit(shell.exit_code);

        if (root->operator.right)
            execute_ast(root->operator.right, data);
    }

    // If the left arm's exit code is 0 (success), execute the right
    // arm too.
    else if (root->type == T_AND) {
        execute_ast(root->operator.left, data);

        if (shell.exit_code == 0)
            execute_ast(root->operator.right, data);
    }

    // If the left arm's exit code is non-zero (failure), execute
    // the right arm too.
    else if (root->type == T_OR) {
        execute_ast(root->operator.left, data);

        if (shell.exit_code != 0)
            execute_ast(root->operator.right, data);
    }

    // If the operator is ampersand, execute left pipeline as background job
    else if (root->type == T_AMPER) {
        TaskData bgdata = data;
        bgdata.fg = false;
        execute_ast(root->operator.left, bgdata);
        if (root->operator.right)
            execute_ast(root->operator.right, data);
    }

    // Execute commands below this node with pipe_handle
    else if (root->type == T_PIPE) {
        pipe_handle(root, data);
    }
}


// Returns E_PARSE_ERROR on parse error and writes its error
// message to error_out.
// On parse error, set exit code to EXIT_FAILURE if it is set
// to 0 (success), otherwise, leave the previous exit code as-is.
ExeResult execute_line(char* line, char **error_out) {
    // Trimming spaces at the start and end of the command.
    trim(line);
    // Return on empty or comment line.
    if (line[0] == '\0' || line[0] == '#')
        return E_SUCCESS;

    TokenArray ta;
    tokenize(line, &ta);

    int e = perform_expansions(&ta, error_out);
    if (e) {
        free_tokens(ta);

        if (shell.exit_code == 0)
            shell.exit_code = EXIT_FAILURE;

        return E_PARSE_ERROR;
    }

    if (ta.len == 0) {
        shell.exit_code = 0;
        free_tokens(ta);
        return E_SUCCESS;
    }

    Node* root = parse(ta.tokens, ta.len, error_out);

    if (root == NULL) {
        free_tokens(ta);

        if (shell.exit_code == 0)
            shell.exit_code = EXIT_FAILURE;

        return E_PARSE_ERROR;
    }

    TaskData data = { .fg = true };

    execute_ast(root, data);

    free_ast(root);
    free_tokens(ta);

    return E_SUCCESS;
}


// Executes a file line by line. Command failures do not stop
// the file execution. Stops at the first parse error; lines
// before the error are already executed.
//
// Returns
// - E_SUCCESS if the file executed successfully.
// - E_PARSE_ERROR if a line contains parse error.
// - E_FILE_ERROR if the file cannot be opened.
ExeResult execute_file(const char* filename) {
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        print_err(filename, strerror(errno));
        shell.exit_code = 127;
        return E_FILE_ERROR;
    }

    char line[MAX_LINE_SIZE];
    size_t line_count = 1;
    char* error_message = NULL;

    while (fgets(line, MAX_LINE_SIZE, file)) {
        ExeResult ex = execute_line(line, &error_message);

        manage_bg_jobs();

        if (ex == E_PARSE_ERROR) {
            fprintf(stderr, "%s: line %zu: %s\n",
                            filename, line_count, error_message);
            fprintf(stderr, "%s: line %zu: '%s'\n",
                            filename, line_count, line);

            free(error_message);
            fclose(file);
            return E_PARSE_ERROR;
        }

        line_count++;
    }

    fclose(file);
    return E_SUCCESS;
}
