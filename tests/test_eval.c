#include <math.h>

#include "test_ut.h"
#include "ieee_fp.h"
#include "eval.h"
#include "compat_eval.h"

// TODO check eval.c
// TODO predefined symbols

typedef unsigned int uint;

bool test_eval_vars() {  // more tests!
    uint varid0 = add_var(12, "a");
    uint varid1 = add_var(32, "b");
    set_var(varid0, get_var(varid0) + 30);
    return ASSERT(get_var(varid0) == 42)
        && ASSERT(get_var(varid0) - 10 == get_var(varid1));
}

bool test_eval_funcall() {
    unsigned int fid0 = add_func("f");
    set_func_args(fid0, listFromArray((char*[1]){ "a" }, 1, sizeof(char *)));
    set_func_body(fid0, listFromArray((Token[3]){ { Arg, 0 }, { Operator, { .op = OAdd } }, { Number, 10 } }, 3, sizeof(Token)));
    struct list_head *arg0 = LIST_CONV(Token, 1, ARG({ { Number, 32.0 } }));
    struct list_head *argv = makeList(sizeof(struct list_head *));
    listAppend(argv, &arg0);
    bool res = ASSERT(call_func(fid0, argv) == 42.0);
    destroyList(arg0);
    destroyList(argv);
    return res;
}

bool test_eval_expr1() {
    // ( 15. - 3. ) * 4.
    struct list_head *tokens = listFromArray((Token[7]){
            { Operator, { .op = OLp } },
            { Number, 15. }, { Operator, { .op = OSub } }, { Number, 3. },
            { Operator, { .op = ORp } },
            { Operator, { .op = OMul } }, { Number, 4. } },
        7, sizeof(Token));
    bool res = ASSERT(eval_expr(tokens, 0) == 48.);
    destroyList(tokens);
    return res;
}

bool test_eval_assignExpr1() {
    unsigned int x_id = add_var(0., "x");
    struct list_head *tokens = listFromArray((Token[5]){
            { Var, { .id = x_id } },
            { Operator, { .op = OAssign } },
            { Number, 15. }, { Operator, { .op = OTDiv } }, { Number, 2. } },
        5, sizeof(Token));
    bool res = ASSERT(eval_expr(tokens, 0) == 7.5)
            && ASSERT(get_var(x_id) == 7.5);
    destroyList(tokens);
    return res;
}

bool test_eval_allArgs() {
    struct list_head *args = makeList(sizeof(struct list_head *));
    struct list_head *arg0 = listFromArray((Token[1]){ { Number, 3. } }, 1, sizeof(Token));
    listAppend(args, &arg0);
    struct list_head *argv = eval_all_args(args);
    bool res = ASSERT(argv->length == 1)
        && ASSERT(GET_DOUBLE(argv, 0) == 3);
    destroyList(argv);
    destroyList(arg0);
    destroyList(args);
    return res;
}

bool test_eval_assignExpr2() {
    unsigned int f_id = add_func("f");
    set_func_args(f_id, listFromArray((char*[1]){ "x" }, 1, sizeof(char *)));
    struct list_head *tokens0 = listFromArray((Token[8]){
            { Func, { .id = f_id } }, { Operator, { .op = OLCall } }, { Arg, { .id = 0 } }, { Operator, { .op = ORCall } },
            { Operator, { .op = OAssign } },
            { Arg, { .id = 0 } }, { Operator, { .op = OMul } }, { Arg, { .id = 0 } } },
        8, sizeof(Token));
    struct list_head *tokens1 = listFromArray((Token[4]){
            { Func, { .id = f_id } }, { Operator, { .op = OLCall } },
            { Number, 3. },
            { Operator, { .op = ORCall } } },
        4, sizeof(Token));
    bool res = ASSERT(eval_expr(tokens0, 0) == 0)
            && ASSERT(eval_expr(tokens1, 0) == 9.);
    destroyList(tokens0);
    destroyList(tokens1);
    return res;
}

PREDEF_ONELINE_FUNC(some_predef_f, args, argv, ARG(GET_DOUBLE(argv, 0) * 2))

bool test_eval_PredefinedFuncs() {
    unsigned int fid = set_func_func(set_func_args(add_func("some_pf"),
                LIST_CONV(char *, 1, { "x" })),
            some_predef_f);
    struct list_head *arg0 = LIST_CONV(Token, 1, ARG({ { Number, 21 } }));
    struct list_head *args = makeList(sizeof(struct list_head *));
    listAppend(args, &arg0);
    bool res = \
            ASSERT(call_func(fid, args) == 42.0);
    destroyList(arg0);
    destroyList(args);
    return res;
}

bool test_eval_RedefineFunc() {
    unsigned int fid = set_func_body(set_func_args(add_func("some_rf"), LIST_CONV(char *, 1, { "arg" })),
            LIST_CONV(Token, 3, ARG({
                    { Arg, { .id = 0 } }, { Operator, { .op = OMul } }, { Number, 2 } })));
    struct list_head *arg0 = LIST_CONV(Token, 1, ARG({ Number, 15 }));
    struct list_head *args = LIST_CONV(struct list_head *, 1, { arg0 });
    bool res = ASSERT(call_func(fid, args) == 30);
    if (!res)
        goto t_e_rf_ret;
    set_func_body(set_func_args(fid, LIST_CONV(char *, 2, ARG({ "a0", "a1" }))),
            LIST_CONV(Token, 3, ARG({
                    { Arg, { .id = 0 } }, { Operator, { .op = OMul } }, { Arg, { .id = 1 } } })));
    struct list_head *arg1 = LIST_CONV(Token, 1, ARG({ Number, -1. }));
    listAppend(args, &arg1);
    res &= ASSERT(call_func(fid, args) == -15);
    destroyList(arg1);
t_e_rf_ret:
    destroyList(arg0);
    destroyList(args);
    return res;
}

bool test_eval_ieeeFp() {
    return ASSERT(ieee_copysign(1.0, -1.0) == -1.0)
        && ASSERT(ieee_copysign(-100.5, 0.3) == 100.5)
        && ASSERT(ieee_copysign(0.0, -1.0) == -0.0)
        && ASSERT(ieee_trunc(23.5) == 23.0)
        && ASSERT(ieee_trunc(-12.1245) == -12.0)
        && ASSERT(ieee_fabs(0.5) == 0.5)
        && ASSERT(ieee_fabs(-12365312) == 12365312)
        && ASSERT(FP_EQ(ieee_fmod(11.5, 3.2), 1.9))
        && ASSERT(FP_EQ(ieee_fmod(0.03, 0.01), 0.0))
        && ASSERT(ieee_fpclassify(12.3) == FP_NORMAL)
        && ASSERT(ieee_fpclassify(0.0) == FP_ZERO)
        && ASSERT(ieee_fpclassify(INFINITY) == ieee_fpclassify(-INFINITY))
        && ASSERT(ieee_fpclassify(INFINITY) == FP_INFINITE)
        && ASSERT(ieee_fpclassify(nan("error")) == FP_NAN);
}

bool test_eval_zeroArglist() {
    unsigned int fid = set_func_body(set_func_args(add_func("zaf"),
                                                   makeList(sizeof(char *))),
                                     LIST_CONV(struct token, 1, ARG({{ Number, {.number = 42.0} }})));
    struct list_head *expr =
        LIST_CONV(struct token, 5,
                  ARG({{Func, {.id = fid} }, {Operator, {.op = OLCall}}, {Operator, {.op = ORCall}}, {Operator, {.op = OAdd}}, {Number, {.number = 12.0}}}));
    return ASSERT(eval_expr(expr, 0) == 54.0);
}

int main() {
    struct eval_state es;
    default_eval_state = &es;
    init_eval_state(default_eval_state);
    init_calc();
    bool res =
        test_eval_vars()
        && test_eval_funcall()
        && test_eval_expr1()
        && test_eval_assignExpr1()
        && test_eval_allArgs()
        && test_eval_assignExpr2()
        && test_eval_PredefinedFuncs()
        && test_eval_RedefineFunc()
        && test_eval_ieeeFp()
        && test_eval_zeroArglist();
    if (res)
        fprintf(stderr, "All ok\n");
    destroy_calc();
    return !res;
}
