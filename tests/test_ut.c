#include "test_ut.h"

bool assert(bool val, char *errstring, char *filename, unsigned int lineno) {
    if (!val) {
        fprintf(stderr, "%s:%d : %s\n", filename, lineno, errstring);
        return false;
    }
    return true;
}

