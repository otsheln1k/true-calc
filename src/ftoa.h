#ifndef FTOA_INCLUDED
#define FTOA_INCLUDED

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#ifdef __PEBBLE__
#include "SmallMaths.h"
#define __pow sm_pow
#else
#define __pow pow
#endif

void ftoa(double, char *, size_t);

#endif

