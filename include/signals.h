#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdbool.h>
#include <signal.h>


extern volatile sig_atomic_t in_readline;

void sigint_handler(int sig);

#endif
