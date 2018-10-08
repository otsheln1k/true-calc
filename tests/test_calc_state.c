#include "test_ut.h"
#include "eval.h"
#include "calc_state.h"

/* GLOBAL */
unsigned int fid;

bool test_cs_1() {
    CalcState *cs = cs_create();
    cs_add_item(cs, (Token){ Number, 15 });
    cs_add_item(cs, (Token){ Operator, { .op = OAdd } });
    cs_add_item(cs, (Token){ Number, 27 });
    bool res = ASSERT(eval_expr(cs_get_expr(cs), 0) == 42.)
            && ASSERT(!strcmp(cs_curr_str(cs), "15+27"));
    if (!res)
        goto test_failed;
    cs_add_item(cs, (Token){ Operator, { .op = OTDiv } });
    cs_add_item(cs, (Token){ Number, 3. });
    res &= ASSERT(eval_expr(cs_get_expr(cs), 0) == 24.)
        && ASSERT(!strcmp(cs_curr_str(cs), "15+27/3"));
test_failed:
    cs_destroy(cs);
    return res;
}

bool test_cs_2() {
    CalcState *cs = cs_create();
    unsigned int fid0 = set_func_body(set_func_args(add_func("f1"), LIST_CONV(char *, 1, { "p0" })), LIST_CONV(Token, 3, ARG({ { Arg, { .id = 0 } }, { Operator, { .op = OAdd } }, { Number, 10 } })));
    cs_add_item(cs, (Token){ Func, { .id = fid0 } });
    cs_add_item(cs, (Token){ Operator, { .op = OLCall } });
    cs_add_item(cs, (Token){ Number, 15 });
    cs_add_item(cs, (Token){ Operator, { .op = ORCall } });
    bool res = ASSERT(eval_expr(cs_get_expr(cs), 0) == 25.)
            && ASSERT(!strcmp(cs_curr_str(cs), "f1(15)"));
    cs_destroy(cs);
    return res;
}

bool test_cs_3() {
    CalcState *cs = cs_create();
    unsigned int vid = add_var(21., "var_0");
    unsigned int cid = add_const(2., "cst_0");
    cs_add_item(cs, (Token){ Var, { .id = vid } });
    cs_add_item(cs, (Token){ Operator, { .op = OAssign } });
    cs_add_item(cs, (Token){ Var, { .id = vid } });
    cs_add_item(cs, (Token){ Operator, { .op = OMul } });
    cs_add_item(cs, (Token){ Const, { .id = cid } });
    bool res = ASSERT(eval_expr(cs_get_expr(cs), 0) == 42.)
            && ASSERT(get_var(vid) == 42.)
            && ASSERT(!strcmp(cs_curr_str(cs), "var_0=var_0*cst_0"));
    cs_destroy(cs);
    return res;
}

char *t_ftoa(double d) {
    static char buf[20];
    ftoa(d, buf, sizeof(buf));
    return buf;
}

bool test_cs_ftoa() {
    char small_buf[9];
    return ASSERT(!strcmp(t_ftoa(42), "42"))
        && ASSERT(!strcmp(t_ftoa(.05), ".05"))
        && ASSERT(!strcmp(t_ftoa(12.34), "12.34"))
        && ASSERT(!strcmp(t_ftoa(11.), "11"))
        && ASSERT(!strcmp(t_ftoa(-100500.25), "-100500.25"))
        && ASSERT(!strcmp(t_ftoa(1e21), "1e21"))
        && (ftoa(-123.5679, small_buf, 9),
            ASSERT(!strcmp(small_buf, "-123.568")))
        && (ftoa(-0.0000123, small_buf, 9),
            ASSERT(!strcmp(small_buf, "-1.23e-5")))
        && (ftoa(12345678, small_buf, 9),
            ASSERT(!strcmp(small_buf, "1.2346e7")));
}

bool test_cs_Interact() {
    CalcState *cs = cs_create();
    unsigned int varid = add_var(15., "var_1");
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 27.);
    cs_interact(cs, TIIPlus);
    cs_interact(cs, TIIVar);
    cs_input_id(cs, varid);
    bool res = ASSERT(eval_expr(cs_get_expr(cs), 0) == 42.)
            && ASSERT(!strcmp(cs_curr_str(cs), "27+var_1"));
    remove_var(varid);
    cs_destroy(cs);
    return res;
}

bool test_cs_InteractiveVar() {
    CalcState *cs = cs_create();
    cs_interact(cs, TIIVar);
    char *n = malloc(4);
    strcpy(n, "v_2");
    unsigned int vid = add_var(0, n);
    cs_input_newid(cs, vid);
    cs_interact(cs, TIIAssign);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 21.);
    cs_eval(cs);
    cs_clear(cs);
    cs_interact(cs, TIIVar);
    cs_input_id(cs, vid);
    cs_interact(cs, TIIMultiply);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 2.);
    bool res = ASSERT(cs_eval(cs) == 42.);
    cs_destroy(cs);
    return res;
}

bool test_cs_Nesting() {
    CalcState *cs = cs_create();
    cs_interact(cs, TIILParen);
    if (!ASSERT(cs->nesting == 1)) { bool res = false; goto t_c_N_ret; }
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 15.5);
    cs_interact(cs, TIIMinus);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 1.5);
    cs_interact(cs, TIIRParen);
    if (!ASSERT(cs->nesting == 0)) { bool res = false; goto t_c_N_ret; }
    cs_interact(cs, TIIBackspace);
    if (!ASSERT(cs->nesting == 1)) { bool res = false; goto t_c_N_ret; }
    cs_interact(cs, TIIRParen);
    cs_interact(cs, TIIMultiply);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 3.);
    bool res = ASSERT(cs_eval(cs) == 42.);
t_c_N_ret:
    cs_destroy(cs);
    return res;
}

bool test_cs_Stack() {
    CalcState *cs = cs_create();
    cs_interact(cs, TIIFunction);
    cs_input_id(cs, fid);
    cs_interact(cs, TIILFuncall);
    bool res = ASSERT(cs->fcalls->length == 1);
    if (!res) goto t_c_S_ret;
    FuncallMark *fmp = LIST_DATA(FuncallMark, cs->fcalls, 0);
    res &= ASSERT(fmp->fid == fid)
        && ASSERT(fmp->nesting_level == 0)
        && ASSERT(fmp->arg_idx == 0);
    if (!res) goto t_c_S_ret;
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 12.);
    cs_interact(cs, TIIComma);
    res &= ASSERT(fmp->arg_idx == 1);
    if (!res) goto t_c_S_ret;
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 29.);
    cs_interact(cs, TIIRFuncall);
    res &= ASSERT(cs->fcalls->length == 0)
        && ASSERT(cs_eval(cs) == 42.);
t_c_S_ret:
    cs_destroy(cs);
    return res;
}

bool test_cs_Scope() {
    CalcState *cs = cs_create();
    cs_interact(cs, TIIFunction);
    unsigned int fid1 = add_func("newf");
    cs_input_id(cs, fid1);          // check cs_input_newid
    cs_interact(cs, TIILFuncall);
    cs_interact(cs, TIIAddArg);
    cs_interact(cs, TIIRFuncall);
    cs_interact(cs, TIIAssign);
    bool res = ASSERT(cs->scope.fid == fid1)
            && ASSERT(cs->scope.offset == 5)
            && ASSERT(cs->scope.nesting_level == 0);
    if (!res) goto t_c_Sc_ret;
    cs_interact(cs, TIIArg);
    cs_input_id(cs, 0);
    cs_interact(cs, TIIFloorDiv);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 2.);
    res &= ASSERT(cs_eval(cs) == 0);
    cs_clear(cs);
    cs_interact(cs, TIIFunction);
    cs_input_id(cs, fid1);
    cs_interact(cs, TIILFuncall);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 85.);
    cs_interact(cs, TIIRFuncall);
    res &= ASSERT(cs_eval(cs) == 42.);
t_c_Sc_ret:
    cs_destroy(cs);
    return res;
}

PREDEF_ONELINE_FUNC(test_func_oneplus, args, argv,
    (GET_DOUBLE(argv, 0) + GET_DOUBLE(argv, 1) + 1.));

#define FINAL_ASSERT(var, expr, mark, ...) { var &= ASSERT(expr); if (!var) { __VA_ARGS__ ; goto mark; } }

bool test_cs_Expect1() {
    CalcState *cs = cs_create();
    bool res = true;
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 126);
    FINAL_ASSERT(res, cs->exp == TEBinaryOperator, t_c_E_ret);
    cs_interact(cs, TIITrueDiv);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E_ret);  // fails here
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 3);
    res &= ASSERT(cs_eval(cs) == 42);
    cs_clear(cs);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E_ret);
    cs_interact(cs, TIIFunction);
    cs_input_id(cs, fid);
    FINAL_ASSERT(res, cs->exp == TEFuncall, t_c_E_ret);
    cs_interact(cs, TIILFuncall);
    FINAL_ASSERT(res, cs->exp == (TEValue | TEFRCall | TEFArgs), t_c_E_ret, fprintf(stderr, "%d\n", cs->exp));
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 12);
    FINAL_ASSERT(res, cs->exp == (TEBinaryOperator | TEFComma | TEFRCall), t_c_E_ret,
            fprintf(stderr, "%d\n", cs->exp));
    cs_interact(cs, TIIComma);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E_ret,
            fprintf(stderr, "%d\n", cs->exp));
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 29);
    FINAL_ASSERT(res, cs->exp == (TEBinaryOperator | TEFComma | TEFRCall), t_c_E_ret);
    cs_interact(cs, TIIRFuncall);
    FINAL_ASSERT(res, cs->exp == TEBinaryOperator, t_c_E_ret);
    res &= ASSERT(cs_eval(cs) == 42);
t_c_E_ret:
    cs_destroy(cs);
    return res;
}

int test_cs_Expect2() {
    CalcState *cs = cs_create();
    bool res = true;
    unsigned int varid1 = add_var(2., "var_1");
    cs_interact(cs, TIILParen);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E2_ret,
            fprintf(stderr, "%d\n", cs->exp));
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 20);
    FINAL_ASSERT(res, cs->exp == (TECloseParen | TEBinaryOperator), t_c_E2_ret,
            fprintf(stderr, "%d\n", cs->exp));
    cs_interact(cs, TIIPlus);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E2_ret);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 1);
    FINAL_ASSERT(res, cs->exp == (TECloseParen | TEBinaryOperator), t_c_E2_ret);
    cs_interact(cs, TIIRParen);
    FINAL_ASSERT(res, cs->exp == TEBinaryOperator, t_c_E2_ret);
    cs_interact(cs, TIIMultiply);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E2_ret);
    cs_interact(cs, TIIVar);
    cs_input_id(cs, varid1);
    FINAL_ASSERT(res, cs->exp == TEBinaryOperator, t_c_E2_ret);  // may have TEAssign
    res &= ASSERT(cs_eval(cs) == 42);
t_c_E2_ret:
    remove_var(varid1);
    cs_destroy(cs);
    return res;
}

int test_cs_Expect3() {
    CalcState *cs = cs_create();
    bool res = true;
    unsigned int fid0 = add_func("f1");
    unsigned int varid = add_var(0, "var_2");
    cs_interact(cs, TIIFunction);
    cs_input_id(cs, fid0);
    FINAL_ASSERT(res, cs->exp == TEFuncall, t_c_E3_ret);
    cs_interact(cs, TIILFuncall);
    FINAL_ASSERT(res, cs->exp == (TEValue | TEFRCall | TEFArgs), t_c_E3_ret,
            fprintf(stderr, "%d\n", cs->exp));
    cs_interact(cs, TIIAddArg);
    cs_input_newarg(cs);
    FINAL_ASSERT(res, cs->exp == (TEFRCall | TEFComma), t_c_E3_ret,
            fprintf(stderr, "%d\n", cs->exp));
    cs_interact(cs, TIIRFuncall);
    FINAL_ASSERT(res, cs->exp == TEAssign, t_c_E3_ret);
    cs_interact(cs, TIIAssign);
    FINAL_ASSERT(res, cs->exp == (TEValue | TEFArgs), t_c_E3_ret);
    cs_interact(cs, TIIArg);
    cs_input_id(cs, 0);
    FINAL_ASSERT(res, cs->exp == TEBinaryOperator, t_c_E3_ret);
    cs_interact(cs, TIIPower);
    FINAL_ASSERT(res, cs->exp == (TEValue | TEFArgs), t_c_E3_ret);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 2);
    FINAL_ASSERT(res, cs->exp == TEBinaryOperator, t_c_E3_ret);
    cs_eval(cs);
    cs_clear(cs);
    cs_interact(cs, TIIFunction);
    cs_input_id(cs, fid0);
    FINAL_ASSERT(res, cs->exp == TEFuncall, t_c_E3_ret);
    cs_interact(cs, TIILFuncall);
    FINAL_ASSERT(res, cs->exp == (TEValue | TEFArgs | TEFRCall), t_c_E3_ret);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 2);
    FINAL_ASSERT(res, cs->exp == (TEBinaryOperator | TEFComma | TEFRCall), t_c_E3_ret);
    cs_interact(cs, TIIMultiply);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E3_ret);
    cs_interact(cs, TIILParen);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E3_ret);
    cs_interact(cs, TIIVar);
    cs_input_id(cs, varid);
    FINAL_ASSERT(res, cs->exp == (TEBinaryOperator | TECloseParen | TEAssign), t_c_E3_ret);
    cs_interact(cs, TIIAssign);
    FINAL_ASSERT(res, cs->exp == TEValue, t_c_E3_ret);
    cs_interact(cs, TIINumber);
    cs_input_number(cs, 10);
    FINAL_ASSERT(res, cs->exp == (TEBinaryOperator | TECloseParen), t_c_E3_ret);
    cs_interact(cs, TIIRParen);
    FINAL_ASSERT(res, cs->exp == (TEBinaryOperator | TEFRCall | TEFComma), t_c_E3_ret,
            fprintf(stderr, "%d\n", cs->exp));
    cs_interact(cs, TIIRFuncall);
    FINAL_ASSERT(res, cs->exp == TEBinaryOperator, t_c_E3_ret);
    res &= ASSERT(cs_eval(cs) == 400);
t_c_E3_ret:
    remove_var(varid);
    cs_destroy(cs);
    return res;
}

int main() {
    init_calc();
    fid = set_func_func(set_func_args(add_func("tf1+"),
                LIST_CONV(char *, 2, ARG({ "x", "y" }))),
            test_func_oneplus);
    bool res = test_cs_1()
            && test_cs_ftoa()
            && test_cs_2()
            && test_cs_3()
            && test_cs_Interact()
            && test_cs_InteractiveVar()
            && test_cs_Nesting()
            && test_cs_Stack()
            && test_cs_Scope()
            && test_cs_Expect1()
            && test_cs_Expect2()
            && test_cs_Expect3();
    fprintf(stderr, res ? "All ok\n" : "Some tests failed\n");
    destroy_calc();
    return !res;
}

