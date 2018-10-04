#include "test_ut.h"
#include "eval.h"

// TODO check eval.c
// TODO predefined symbols
// TODO eval_angle

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
    set_func_args(fid0, convertedArray((char*[1]){ "a" }, 1, sizeof(char *)));
    set_func_body(fid0, convertedArray((Token[3]){ { Arg, 0 }, { Operator, { .op = OAdd } }, { Number, 10 } }, 3, sizeof(Token)));
    // array arg0 = convertedArray((double[1]){ 32.0 }, 1, sizeof(double));
    array arg0 = ARRAY_CONV(Token, 1, ARG({ { Number, 32.0 } }));
    array argv = makeArray(sizeof(array));
    arrayAppend(argv, &arg0);
    bool res = ASSERT(call_func(fid0, argv) == 42.0);
    destroyArray(arg0);
    destroyArray(argv);
    return res;
}

bool test_eval_expr1() {
    array tokens = convertedArray((Token[7]){
            { Operator, { .op = OLp } },
            { Number, 15. }, { Operator, { .op = OSub } }, { Number, 3. },
            { Operator, { .op = ORp } },
            { Operator, { .op = OMul } }, { Number, 4. } },
        7, sizeof(Token));
    bool res = ASSERT(eval_expr(tokens, 0) == 48.);
    destroyArray(tokens);
    return res;
}

bool test_eval_assignExpr1() {
    unsigned int x_id = add_var(0., "x");
    array tokens = convertedArray((Token[5]){
            { Var, { .id = x_id } },
            { Operator, { .op = OAssign } },
            { Number, 15. }, { Operator, { .op = OTDiv } }, { Number, 2. } },
        5, sizeof(Token));
    bool res = ASSERT(eval_expr(tokens, 0) == 7.5)
            && ASSERT(get_var(x_id) == 7.5);
    destroyArray(tokens);
    return res;
}

bool test_eval_allArgs() {
    array args = makeArray(sizeof(array));
    array arg0 = convertedArray((Token[1]){ { Number, 3. } }, 1, sizeof(Token));
    arrayAppend(args, &arg0);
    array argv = eval_all_args(args);
    bool res = ASSERT(getArrayLength(argv) == 1)
            && ASSERT(*(double *)getArrayItemValue(argv, 0) == 3);
    destroyArray(argv);
    destroyArray(arg0);
    destroyArray(args);
    return res;
}

bool test_eval_assignExpr2() {
    unsigned int f_id = add_func("f");
    set_func_args(f_id, convertedArray((char*[1]){ "x" }, 1, sizeof(char *)));
    array tokens0 = convertedArray((Token[8]){
            { Func, { .id = f_id } }, { Operator, { .op = OLCall } }, { Arg, { .id = 0 } }, { Operator, { .op = ORCall } },
            { Operator, { .op = OAssign } },
            { Arg, { .id = 0 } }, { Operator, { .op = OMul } }, { Arg, { .id = 0 } } },
        8, sizeof(Token));
    array tokens1 = convertedArray((Token[4]){
            { Func, { .id = f_id } }, { Operator, { .op = OLCall } },
            { Number, 3. },
            { Operator, { .op = ORCall } } },
        4, sizeof(Token));
    bool res = ASSERT(eval_expr(tokens0, 0) == 0)
            && ASSERT(eval_expr(tokens1, 0) == 9.);
    destroyArray(tokens0);
    destroyArray(tokens1);
    return res;
}

PREDEF_ONELINE_FUNC(some_predef_f, args, argv, ARG(GET_DOUBLE(argv, 0) * 2))

bool test_eval_PredefinedFuncs() {
    unsigned int fid = set_func_func(set_func_args(add_func("some_pf"),
                ARRAY_CONV(char *, 1, { "x" })),
            some_predef_f);
    array arg0 = ARRAY_CONV(Token, 1, ARG({ { Number, 21 } }));
    array args = makeArray(sizeof(array));
    arrayAppend(args, &arg0);
    bool res = \
            ASSERT(call_func(fid, args) == 42.0);
    destroyArray(arg0);
    destroyArray(args);
    return res;
}

bool test_eval_RedefineFunc() {
    unsigned int fid = set_func_body(set_func_args(add_func("some_rf"), ARRAY_CONV(char *, 1, { "arg" })),
            ARRAY_CONV(Token, 3, ARG({
                    { Arg, { .id = 0 } }, { Operator, { .op = OMul } }, { Number, 2 } })));
    array arg0 = ARRAY_CONV(Token, 1, ARG({ Number, 15 }));
    array args = ARRAY_CONV(array, 1, { arg0 });
    bool res = ASSERT(call_func(fid, args) == 30);
    if (!res)
        goto t_e_rf_ret;
    set_func_body(set_func_args(fid, ARRAY_CONV(char *, 2, ARG({ "a0", "a1" }))),
            ARRAY_CONV(Token, 3, ARG({
                    { Arg, { .id = 0 } }, { Operator, { .op = OMul } }, { Arg, { .id = 1 } } })));
    array arg1 = ARRAY_CONV(Token, 1, ARG({ Number, -1. }));
    arrayAppend(args, &arg1);
    res &= ASSERT(call_func(fid, args) == -15);
    destroyArray(arg1);
t_e_rf_ret:
    destroyArray(arg0);
    destroyArray(args);
    return res;
}

int main() {
    init_calc();
    bool res =
        test_eval_vars()
     && test_eval_funcall()
     && test_eval_expr1()
     && test_eval_assignExpr1()
     && test_eval_allArgs()
     && test_eval_assignExpr2()
     && test_eval_PredefinedFuncs()
     && test_eval_RedefineFunc();
    if (res)
        fprintf(stderr, "All ok\n");
    destroy_calc();
    return !res;
}
