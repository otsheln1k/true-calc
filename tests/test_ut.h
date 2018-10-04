#ifndef TESTUT_INCLUDED
#define TESTUT_INCLUDED

#include <stdio.h>

#define ARG(...) __VA_ARGS__

typedef char bool;
#define true 1
#define false 0

bool assert(bool val, char *errstring, char *filename, unsigned int lineno);

#define ASSERT(expr) assert(expr, "Assertion failed: "#expr, __FILE__, __LINE__)

#endif

