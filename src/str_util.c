#include <string.h>
#include <stdio.h>

#include "str_util.h"
#include "util.h"

#define START_BUFFER_SIZE 128


static void ensure_capacity(String* str, size_t n) {
    size_t cap = str->_cap;

    while ((str->len + n) >= cap)
        cap *= 2;

    if (cap != str->_cap) {
        str->data = srealloc(str->data, cap);
        str->_cap = cap;
    }
}


String new_string(void) {
    String str;

    str.data = smalloc(START_BUFFER_SIZE);
    str._cap = START_BUFFER_SIZE;
    str.len = 0;
    str.data[0] = '\0';

    return str;
}


void add_chr_to_str(String *str, char ch) {
    ensure_capacity(str, 1);
    str->data[str->len++] = ch;
    str->data[str->len] = '\0';
}


void add_span_to_str(String *str, const char *start, const char *end) {
    if (end - start <= 0) return;
    size_t count = end - start;

    ensure_capacity(str, count);

    for (const char *iter = start; iter != end; ++iter) {
        str->data[str->len++] = *iter;
    }
    str->data[str->len] = '\0';
}


void add_slice_to_str(String *str, const char *buffer) {
    size_t count = strlen(buffer);
    if (count == 0) return;

    ensure_capacity(str, count);

    for (size_t i = 0; i < count; ++i)
        str->data[str->len++] = buffer[i];

    str->data[str->len] = '\0';
}


void add_int_to_str(String *str, int num) {
    char buf[16];
    snprintf(buf, 16, "%d", num);
    size_t buf_len = strlen(buf);

    ensure_capacity(str, buf_len);

    for (char* ch = buf; *ch; ++ch)
        str->data[str->len++] = *ch;

    str->data[str->len] = '\0';
}


void clear_str(String *str) {
    str->len = 0;
    str->data[0] = '\0';
}


String from_argv(char** argv) {
    String str = new_string();

    for (size_t i = 0; argv[i]; ++i) {
        if (i > 0) add_chr_to_str(&str, ' ');

        add_slice_to_str(&str, argv[i]);
    }

    return str;
}
