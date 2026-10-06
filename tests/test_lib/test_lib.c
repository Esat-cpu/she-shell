#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "test_lib.h"

#define GREEN  "\033[0;32m"
#define RED    "\033[0;31m"
#define RESET  "\033[0m"

#define ARR_STR_BUFF_SIZE 4096


static size_t tests = 0;
static size_t test_success = 0;
static size_t test_fail = 0;

static bool success_flag = false;
static bool fail_flag = false;


// Print the values of a string array to output buffer.
// `args` must be a NULL-terminated array and,
// `out` must have enough space for the result.
static void
sprint_arr(char** args, char* out) {
    if (args == NULL) {
        snprintf(out, ARR_STR_BUFF_SIZE, "(null)");
        return;
    }

    if (args[0] == NULL) {
        snprintf(out, ARR_STR_BUFF_SIZE, "{}");
        return;
    }

    strcpy(out, "{");
    for (int i = 0; args[i]; ++i) {
        strcat(out, "\"");
        strncat(out, args[i], ARR_STR_BUFF_SIZE - strlen(out) - 4);
        strcat(out, "\", ");
    }
    strcat(out, "\b\b}");
}


// Asserts that two strings are equal.
void
assert_eq_str(const char* str1, const char* str2, const char* func, const char* file, int line) {
    bool equal = (str1 && str2) ? strcmp(str1, str2) == 0 : (!str1 && !str2);

    if (!str1) str1 = "(null)";
    if (!str2) str2 = "(null)";

    if (equal) {
        success_flag = true;
    }
    else {
        fail_flag = true;

        fprintf(stderr, RED "[!] FAIL at %s:%d: %s\n\"%s\" is not \"%s\"!\n" RESET,
                file, line, func, str1, str2);
    }
}


// Asserts that two long integers are equal.
void
assert_eq_long(long num1, long num2, const char* func, const char* file, int line) {
    if (num1 == num2) {
        success_flag = true;
    }
    else {
        fail_flag = true;

        fprintf(stderr, RED "[!] FAIL at %s:%d: %s\n%ld is not %ld!\n" RESET,
                file, line, func, num1, num2);
    }
}


// Asserts that two string arrays are equal.
void
assert_eq_str_arr(char** arr1, char** arr2, const char* func, const char* file, int line) {
    const char* fail_message = RED "[!] FAIL at %s:%d: %s\n%s is not %s!\n" RESET;

    char arr1_str[ARR_STR_BUFF_SIZE];
    char arr2_str[ARR_STR_BUFF_SIZE];
    sprint_arr(arr1, arr1_str);
    sprint_arr(arr2, arr2_str);


    if (arr1 == NULL && arr2 == NULL) {
        success_flag = true;
        return;
    }

    if (arr1 == NULL || arr2 == NULL) {
        fail_flag = true;

        fprintf(stderr, fail_message, file, line, func, arr1_str, arr2_str);
        return;
    }


    size_t i;
    for (i = 0; arr1[i] && arr2[i]; i++) {
        if (strcmp(arr1[i], arr2[i]) != 0) {
            fail_flag = true;

            fprintf(stderr, fail_message, file, line, func, arr1_str, arr2_str);
            return;
        }
    }

    // the case where one array is shorter than the other
    if (arr1[i] != NULL || arr2[i] != NULL) {
        fprintf(stderr, fail_message, file, line, func, arr1_str, arr2_str);
        fail_flag = true;
        return;
    }

    success_flag = true;
}


// Prints statistics for all tests that were run.
// If no tests failed, prints an [OK] message.
static void
end(const char* name) {
    printf("Ran %zu %s test(s).\n", tests, name);
    printf("Success: %zu  |  Fail: %zu\n", test_success, test_fail);
    if (test_fail == 0)
        printf("[" GREEN "OK" RESET "] %s test successful.\n", name);
    printf("-------------------------------------------------------\n");
}


typedef void (*test_callback_t)(void);

// Runs each callback and counts it as a test only if it sets a success or failure flag.
int
run_tests_impl(const char* name, const test_callback_t funcs[], size_t count) {
    for (size_t i = 0; i < count; ++i) {
        success_flag = false;
        fail_flag    = false;

        funcs[i]();
        tests++;

        if (fail_flag)
            test_fail++;
        else if (success_flag)
            test_success++;
        else
            tests--;
    }

    end(name);

    if (test_fail > 0)  return EXIT_FAILURE;
    else                return EXIT_SUCCESS;
}
