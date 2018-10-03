#include "eval.h"

array vars;
array consts;
array funcs;

void arrayAppendToken(array arr, Token token) {
    arrayAppend(arr, &token);
}

void init_calc() {
    vars = makeArray(sizeof(NamedValue));
    consts = makeArray(sizeof(NamedValue));
    /*
       add_const(0, "_angles");  // 0→°; 1→rad
       add_const(10, "_history");
       unsigned int pi_id = add_const(M_PI, "π");
       */
    funcs = makeArray(sizeof(DefinedFunction));
    /*
       set_func_args(add_func("use_deg"), makeArray(sizeof(char *)));
       set_func_args(add_func("use_rad"), makeArray(sizeof(char *)));
       set_func_args(add_func("angle", ARRAY_CONV(char *, 1, { "x" })));
       set_func_args(add_func("set_history_size"), ARRAY_CONV(char *, 1, { "hsz" }));
       set_func_args(add_func("his"), ARRAY_CONV(char *, 1, { "n" }));
       set_func_body(set_func_args(add_func("deg2rad"), ARRAY_CONV(char *, 1, { "a_deg" })),
       ARRAY_CONV(Token, 5, ARG({
       { Const, { .id = pi_id } }, { Operator, { .op = OMul } },
       { Arg, { .id = 0 } }, { Operator, { .op = OTDiv } },
       { Number, { .number = 180 } }
       })));
       set_func_body(set_func_args(add_func("rad2deg"), ARRAY_CONV(char *, 1, { "a_rad" })),
       ARRAY_CONV(Token, 5, ARG({
       { Arg, { .id = 0 } }, { Operator, { .op = OTDiv } },
       { Const, { .id = pi_id } }, { Operator, { .op = OMul } },
       { Number, { .number = 180 } }
       })));
    // TODO builtin functions here
    */
}

void destroy_calc() {
    destroyArray(vars);
    destroyArray(consts);
    FOR_ARRAY_COUNTER(funcs, fn, DefinedFunction, func) {
        if (func->args != NULL)
            destroyArray(func->args);
        if (!func->predef && func->body.tokens != NULL)
            destroyArray(func->body.tokens);
    }
    destroyArray(funcs);
}

double eval_atom(Token atom) {
    switch (atom.type) {
        case Var:
            return get_var(atom.value.id);
        case Const:
            return get_const(atom.value.id);
        case Number:
            return atom.value.number;
        default:
            return 0;
    }
}

unsigned int count_var() {
    return getArrayLength(vars);
}

unsigned int count_const() {
    return getArrayLength(consts);
}

unsigned int count_func() {
    return getArrayLength(funcs);
}

unsigned int add_var(double init_val, char *name) {
    NamedValue nv = { name, init_val };
    /*
    FOR_ARRAY_COUNTER(vars, vidx, NamedValue, var)
        if (var->name == NULL) {
            *var = nv;
            return vidx;
        }
    */
    arrayAppend(vars, &nv);
    return getArrayLength(vars) - 1;
}

double get_var(unsigned int varid) {  // assumes that var with id varid already exists
    return ((NamedValue *)getArrayItemValue(vars, varid))->value;
}

char *get_var_name(unsigned int varid) {
    return ((NamedValue *)getArrayItemValue(vars, varid))->name;
}

void set_var(unsigned int varid, double value) {
    ((NamedValue *)getArrayItemValue(vars, varid))->value = value;
}

void remove_var(unsigned int varid) {
    // *(NamedValue *)getArrayItemValue(vars, varid) = (NamedValue){ NULL, 0 };
    arrayRemove(vars, varid);
}

unsigned int add_const(double value, char *name) {
    NamedValue nc = { name, value };
    /*
    FOR_ARRAY_COUNTER(consts, cidx, NamedValue, cnst)
        if (cnst->name == NULL) {
            *cnst = nv;
            return cidx;
        }
    */
    arrayAppend(consts, &nc);
    return getArrayLength(consts) - 1;
}

double get_const(unsigned int cid) {
    return ((NamedValue *)getArrayItemValue(consts, cid))->value;
}

char *get_const_name(unsigned int cid) {
    return ((NamedValue *)getArrayItemValue(consts, cid))->name;
}

void set_const(unsigned int cid, double val) {
    ((NamedValue *)getArrayItemValue(consts, cid))->value = val;
}

void remove_const(unsigned int cid) {
    // *(NamedValue *)getArrayItemValue(consts, cid) = (NamedValue){ NULL, 0 };
    arrayRemove(consts, cid);
}

unsigned int add_func(char *name) {
    DefinedFunction defun = { name, NULL, true, { NULL } };
    arrayAppend(funcs, &defun);
    return getArrayLength(funcs) - 1;
}

unsigned int set_func_args(unsigned int fid, array args) {
    DefinedFunction *func = getArrayItemValue(funcs, fid);
    if (func->args != NULL)
        destroyArray(func->args);
    func->args = args;
    return fid;
}

unsigned int set_func_body(unsigned int fid, array body) {
    DefinedFunction *func = getArrayItemValue(funcs, fid);
    if (!func->predef && func->body.tokens != NULL)
        destroyArray(func->body.tokens);
    func->body.tokens = body;
    func->predef = false;
    return fid;
}

unsigned int set_func_func(unsigned int fid, PredefFunc f) {
    DefinedFunction *func = getArrayItemValue(funcs, fid);
    if (!func->predef && func->body.tokens != NULL)
        destroyArray(func->body.tokens);
    func->body.f = f;
    func->predef = true;
    return fid;
}

unsigned int get_func_argc(unsigned int fid) {
    return getArrayLength(((DefinedFunction *)getArrayItemValue(funcs, fid))->args);
}

char *get_func_arg_name(unsigned int fid, unsigned int arg_idx) {
    return *(char **)getArrayItemValue(get_func_args(fid), arg_idx);
}

array get_func_args(unsigned int fid) {
    DefinedFunction *funcp = getArrayItemValue(funcs, fid);
    if (funcp->args == NULL)
        funcp->args = makeArray(sizeof(char *));
    return funcp->args;
}

char *get_func_name(unsigned int fid) {
    return ((DefinedFunction *)getArrayItemValue(funcs, fid))->name;
}

array eval_all_args(array args) {
    array res = makeArray(sizeof(double));
    FOR_ARRAY_COUNTER(args, argn, array, argv) {
        double val = eval_expr(*argv, 0);
        arrayAppend(res, &val);
    }
    return res;
}

double call_func(unsigned int fid, array argv) {
    DefinedFunction *func = getArrayItemValue(funcs, fid);
    if (func->predef)
        return func->body.f(argv);
    array argval = eval_all_args(argv);
    array body = arrayCopy(func->body.tokens);
    unsigned int len = getArrayLength(body);
    unsigned int idx = 0;
    for (Token *itmp = (Token *)getArrayItemValue(body, 0);
            idx < len;
            itmp = (Token *)getArrayNextItem(itmp)) {
        if (itmp->type == Arg) {
            itmp->type = Number;
            itmp->value.number = *(double *)getArrayItemValue(argval, itmp->value.id);
        }
        idx += 1;
    }
    double res = eval_expr(body, 0);
    destroyArray(body);
    destroyArray(argval);
    return res;
}

void remove_func(unsigned int fid) {
    DefinedFunction *func = getArrayItemValue(funcs, fid);
    if (func->args != NULL)
        destroyArray(func->args);
    if (!func->predef && func->body.tokens != NULL)
        destroyArray(func->body.tokens);
    arrayRemove(funcs, fid);
}

bool is_predefined_func(unsigned int fid) {
    return ((DefinedFunction *)getArrayItemValue(funcs, fid))->predef;
}

double eval_angle(double angle) {
    return (get_const(0) ? angle : M_PI * angle / 180);
}

double eval_operation(OperatorType operation, double op1, double op2) {
    switch (operation) {
        case OMul:
            return op1 * op2;
        case OTDiv:  // `true´ div, 3/2 -> 1.5
            return op1 / op2;
        case OFDiv:
            return (double)((long)op1 / (long)op2);
        case OMod:
            return (double)((long)op1 % (long)op2);
        case OPow:
            return __pow(op1, op2);
        case OAdd:
            return op1 + op2;
        case OSub:
            return op1 - op2;
        default:
            return 0;
    }
}

double eval_uoperation(UOperatorType uoperation, double op) {
    switch (uoperation) {
        case UPlus:
            return op;
        case UMinus:
            return -op;
        default:
            return 0;
    }
}

LValue eval_lvalue(array tokens) {
    unsigned int id = GET_TOKEN(tokens, 0).value.id;
    if (getArrayLength(tokens) == 1)
        return (LValue){
            id,
                false
        };
    else {  // assume that tokens[1] is an OLCall
        /* from here goes the DEFUN */
        // DefinedFunction defun = { NULL, NULL, NULL };
        /*
           array args = makeArray(sizeof(char *));
           void *item = NULL;
           for (Token *iter = (Token *)getArrayItemValue(tokens, 2); ((Token *)getArrayNextItem(iter))->type != ORCall; iter = (Token *)getArrayNextItem(getArrayNextItem(iter)))
           arrayAppend(args, &item);
           */
        /* end of DEFUN */
        return (LValue){
            id, true
        };
    }
}

LValue eval_assign(LValue lvalue, array rvalue) {
    if (lvalue.is_func) {
        set_func_body(lvalue.id, rvalue);
    } else {
        set_var(lvalue.id, eval_expr(rvalue, 0));
    }
    return lvalue;
}

double eval_funcall(unsigned int fid, array args) {
    return call_func(fid, args);
}

double eval_expr(array tokens, int operator_order) {
    unsigned int tlen = getArrayLength(tokens);
    if (tlen == 1)
        return eval_atom(GET_TOKEN(tokens, 0));
    int nested = 0;
    Token *otoken = NULL;
    unsigned int oindex = 0;
    bool par_ok = false;
    unsigned int indx = 0;
    for (Token *iter = getArrayItemValue(tokens, 0); indx < tlen; iter = (Token *)getArrayNextItem(iter)) {
        if (iter->type == Operator) {
            switch (iter->value.op) {
                case OAssign:
                    if (!nested && operator_order == 0)
                    { otoken = iter; oindex = indx; }
                    break;
                case OAdd:
                case OSub:
                    if (!nested && operator_order == 1)
                    { otoken = iter; oindex = indx; }
                    break;
                case OMul:
                case OTDiv:
                case OFDiv:
                case OMod:
                    if (!nested && operator_order == 2)
                    { otoken = iter; oindex = indx; }
                    break;
                case OPow:
                    if (!nested && operator_order == 3)
                    { otoken = iter; oindex = indx; }
                    break;
                case OLCall:  // no break!
                    if (!nested && operator_order == 5)
                    { otoken = iter; oindex = indx; }
                case OLp:
                    nested += 1;
                    break;
                case ORCall:
                case ORp:
                    nested -= 1;
                    break;
                default:
                    break;
            }
        } else if (iter->type == UOperator && !nested && operator_order == 4)
        { otoken = iter; oindex = indx; }
        if (!(iter->type == Operator && (iter->value.op == ORp || iter->value.op == ORCall)) && !par_ok && !nested)
            par_ok = true;
        indx += 1;
    }
    if (!par_ok) {
        array xpr = arraySlice(tokens, 1, tlen - 1);
        double res = eval_expr(xpr, operator_order);
        destroyArrayHeader(xpr);
        return res;
    }
    if (otoken == NULL)  // no such token
        return eval_expr(tokens, operator_order + 1);
    if (otoken->type == UOperator) {
        array xpr = arraySlice(tokens, oindex + 1, tlen);
        double res = eval_uoperation(otoken->value.uop, eval_expr(xpr, 0));
        destroyArrayHeader(xpr);
        return res;
    } else if (otoken->type == Operator && otoken->value.op == OLCall) {
        array args = makeArray(sizeof(array));
        unsigned int nesting = 0;
        unsigned int sindex = oindex;
        unsigned int iindex = oindex + 1;
        array curarg;
        FOR_ARRAY(tokens, iindex, Token, iter)
            if (iter->type == Operator)
                switch (iter->value.op) {
                    case OLCall:
                        nesting += 1;
                        break;
                    case ORCall:
                        if (nesting)
                            nesting -= 1;
                        else {
                            curarg = arraySlice(tokens, sindex + 1, iindex);
                            arrayAppend(args, &curarg);
                            // destroyArrayHeader(curarg);
                            goto eval_f;
                        }
                        break;
                    case OComma:
                        if (!nesting) {
                            curarg = arraySlice(tokens, sindex + 1, iindex);
                            arrayAppend(args, &curarg);
                            // destroyArrayHeader(curarg);
                            sindex = iindex;
                        }
                        break;
                    default:
                        break;
                }
eval_f:;
       double res = eval_funcall(GET_TOKEN(tokens, oindex - 1).value.id, args);
       FOR_ARRAY_COUNTER(args, argn, array, arg)
           destroyArrayHeader(*arg);
       destroyArray(args);
       return res;
    }
    if (otoken->type == Operator && otoken->value.op == OAssign) {
        array lval = arraySlice(tokens, 0, oindex);
        array rval = arrayDetached(ARRAY_TAIL(tokens, oindex + 1));
        LValue res = eval_assign(eval_lvalue(lval), rval);
        destroyArrayHeader(lval);
        return res.is_func ? 0 : get_var(res.id);
    } else {
        array op1 = arraySlice(tokens, 0, oindex);
        array op2 = arraySlice(tokens, oindex + 1, tlen);
        double res = eval_operation(otoken->value.op, eval_expr(op1, 0), eval_expr(op2, 0));
        destroyArrayHeader(op1);
        destroyArrayHeader(op2);
        return res;
    }
}

