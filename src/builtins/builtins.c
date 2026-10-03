#include <stddef.h>

#include "builtins/builtins.h"
#include "builtins/cd.h"
#include "builtins/exit_builtin.h"
#include "builtins/set.h"
#include "builtins/jobs.h"
#include "builtins/bg.h"
#include "builtins/fg.h"
#include "builtins/export.h"
#include "builtins/unset.h"


struct Builtin builtins[] = {
    { .cmd_name = "cd",     .func = cd           },
    { .cmd_name = "exit",   .func = exit_builtin },
    { .cmd_name = "set",    .func = set },
    { .cmd_name = "jobs",   .func = jobs },
    { .cmd_name = "bg",     .func = bg },
    { .cmd_name = "fg",     .func = fg },
    { .cmd_name = "export", .func = export },
    { .cmd_name = "unset",  .func = unset },
    { NULL, NULL },
};
