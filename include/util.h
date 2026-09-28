#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>


void trim(char *str);

void print_err(const char* head, const char* body);

void* smalloc(size_t size);
void* srealloc(void* ptr, size_t new_size);
void* scalloc(size_t size);

char* sstrdup(const char *from);

#endif
