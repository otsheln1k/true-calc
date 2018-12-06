#include "list.h"
#include "SmallMaths.h"
#include "eval.h"
#include "calc_lib.h"

#define ARG(...) __VA_ARGS__

PREDEF_ONELINE_FUNC(predef_sin, args, argv,
                    ARG(sm_sin(GET_DOUBLE(argv, 0))));

PREDEF_ONELINE_FUNC(predef_cos, args, argv,
                    ARG(sm_cos(GET_DOUBLE(argv, 0))));

PREDEF_ONELINE_FUNC(predef_tan, args, argv,
                    ARG(sm_sin(GET_DOUBLE(argv, 0)) / sm_cos(GET_DOUBLE(argv, 0))));

#define ADD_CONST(e, val, name)                     \
    add_named_value_es((e)->consts, val, name, 1)

static void cl_lib_load(struct eval_state *e) {
    FUNCDEF_ARGS(e, "sin", 1, { "a_rad" }, predef_sin);
    FUNCDEF_ARGS(e, "cos", 1, { "a_rad" }, predef_cos);
    FUNCDEF_ARGS(e, "tan", 1, { "a_rad" }, predef_tan);
    ADD_CONST(e, M_PI, "pi");
    ADD_CONST(e, M_PI / 180, "deg2rad");
    ADD_CONST(e, 180 / M_PI, "rad2deg");
}

static double predef_load(struct eval_state *e,
                          struct list_head *args) {
    cl_lib_load(e);
    return 0;
}

void cl_lib_init(struct eval_state *e) {
    FUNCDEF(e, "lib_load", makeList(sizeof(char *)), predef_load);
}

