#include "array.h"
#include "SmallMaths.h"
#include "eval.h"
#include "calc_lib.h"

#define ARG(...) __VA_ARGS__

PREDEF_ONELINE_FUNC(predef_sin, args, argv,
        ARG(sm_sin(GET_DOUBLE(argv, 0))))

PREDEF_ONELINE_FUNC(predef_cos, args, argv,
        ARG(sm_cos(GET_DOUBLE(argv, 0))))

PREDEF_ONELINE_FUNC(predef_tan, args, argv,
        ARG(sm_sin(GET_DOUBLE(argv, 0)) / sm_cos(GET_DOUBLE(argv, 0))))

static void cl_lib_load() {
    FUNCDEF_ARGS("sin", 1, { "a_rad" }, predef_sin);
    FUNCDEF_ARGS("cos", 1, { "a_rad" }, predef_cos);
    FUNCDEF_ARGS("tan", 1, { "a_rad" }, predef_tan);
    add_const(M_PI, "pi");
    add_const(M_PI / 180, "deg2rad");
    add_const(180 / M_PI, "rad2deg");
}

static double predef_load(array args) {
    cl_lib_load();
    return 0;
}

void cl_lib_init() {
    FUNCDEF("lib_load", makeArray(sizeof(char *)), predef_load);
}

