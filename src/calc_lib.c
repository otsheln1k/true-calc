#include <stdlib.h>
#include <string.h>

#include <pebble.h>

#include "list.h"
#include "SmallMaths.h"
#include "eval.h"
#include "calc_state.h"
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


// TRIGLIB
    
static void triglib_load(struct eval_state *e) {
    FUNCDEF_ARGS(e, "sin", 1, { "a_rad" }, predef_sin);
    FUNCDEF_ARGS(e, "cos", 1, { "a_rad" }, predef_cos);
    FUNCDEF_ARGS(e, "tan", 1, { "a_rad" }, predef_tan);
    ADD_CONST(e, M_PI, "pi");
    ADD_CONST(e, M_PI / 180, "deg2rad");
    ADD_CONST(e, 180 / M_PI, "rad2deg");
}

static double predef_load(struct eval_state *e,
                          struct list_head *args) {
    triglib_load(e);
    return 0;
}

void init_triglib(struct eval_state *e) {
    FUNCDEF(e, "lib_load", makeList(sizeof(char *)), predef_load);
}


// CONSTLIB

#define SAVED_CONST_NAME_LEN_MAX 16

void save_const(struct eval_state *e,
                unsigned int cid) {
    int32_t idx = 0;
    if (!persist_exists(0))
        persist_write_int(0, 1);
    else {
        idx = persist_read_int(0);
        persist_write_int(0, ++idx);
    }
    struct named_value *cst = GET_CONST(e, cid);
    double val = cst->value;
    persist_write_data(2 * idx - 1, &val, sizeof(double));
    char buf[SAVED_CONST_NAME_LEN_MAX];
    strncpy(buf, cst->name, sizeof(buf));
    persist_write_string(2 * idx, buf);
}

unsigned int load_const(struct eval_state *e) {
    if (!persist_exists(0))
        return 0;
    int32_t c = persist_read_int(0);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%ld", c);
    for (int32_t i = 1; i <= c; i++) {
        if (!(persist_exists(i * 2 - 1) && persist_exists(i * 2))) {
            persist_write_int(0, 1 - i);
            return i - 1;
        }
        double val;
        persist_read_data(i * 2 - 1, &val, sizeof(double));
        char buf[SAVED_CONST_NAME_LEN_MAX];
        persist_read_string(i * 2, buf, sizeof(buf));
        cs_add_const(
            (struct calc_state *)((unsigned char *)e
                                  - offsetof(struct calc_state, e)),
            val, buf);
    }
    return c;
}

void clear_const() {
    if (!persist_exists(0))
        return;
    int32_t c = persist_read_int(0);
    for (int32_t i = 0; i < c; i++) {
        persist_delete(i * 2 + 1);
        persist_delete((i + 1) * 2);
    }
    persist_write_int(0, 0);
}

static double predef_savec(struct eval_state *e,
                           struct list_head *args) {
    if (!args->length)
        return -1.;
    Token *tokp = GET_PTOKEN(LIST_REF(struct list_head *, args, 0), 0);
    if (tokp->type != Const)
        return -1.;
    save_const(e, tokp->value.id);
    return 0;
}

static double predef_loadc(struct eval_state *e,
                           struct list_head *args) {
    return (double)load_const(e);
}

static double predef_clearc(struct eval_state *e,
                            struct list_head *args) {
    clear_const();
    return 0;
}

void init_constlib(struct eval_state *e) {
    FUNCDEF_ARGS(e, "save_cst", 1, { "cst" }, predef_savec);
    FUNCDEF(e, "load_cst", makeList(sizeof(char *)), predef_loadc);
    FUNCDEF(e, "clear_cst", makeList(sizeof(char *)), predef_clearc);
}
