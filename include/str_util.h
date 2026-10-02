#ifndef STR_UTIL_H
#define STR_UTIL_H

#include <stddef.h>


typedef struct {
    char* data;
    size_t len;
    size_t cap;
} String;


String new_string(void);

void add_chr_to_str(String *str, char ch);

void add_slice_to_str(String *str, const char *buffer);

void add_span_to_str(String *str, const char *start, const char *end);

void add_int_to_str(String *str, int num);

void clear_str(String *str);

#endif
