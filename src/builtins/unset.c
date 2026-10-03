#include <stdlib.h>

#include "builtins/unset.h"


int unset(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        unsetenv(argv[i]);
    }

    return 0;
}
