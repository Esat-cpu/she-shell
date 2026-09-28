#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "util.h"
#include "shell.h"


void trim(char *str) {
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);

    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}


void print_err(const char* head, const char* body) {
    if (body)
        fprintf(stderr, "%s: %s: %s\n", shell.name, head, body);
    else
        fprintf(stderr, "%s: %s\n", shell.name, head);
}


void* smalloc(size_t size) {
    void* p = malloc(size);

    if (!p) {
        print_err("malloc", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return p;
}


void* scalloc(size_t size) {
    void* p = malloc(size);

    if (!p) {
        print_err("malloc", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(p, 0, size);

    return p;
}


void* srealloc(void* ptr, size_t new_size) {
    void* p = realloc(ptr, new_size);

    if (!p) {
        print_err("realloc", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return p;
}


char* sstrdup(const char* from) {
    char* str = strdup(from);

    if (!str) {
        print_err("strdup", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return str;
}
