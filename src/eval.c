#include "eval.h"

struct list_head *vars;
struct list_head *consts;
struct list_head *funcs;

void listAppendToken(struct list_head *arr, Token token) {
    listAppend(arr, &token);
}

void init_calc() {
    vars = makeList(sizeof(NamedValue));
    consts = makeList(sizeof(NamedValue));
    /*
       add_const(0, "_angles");  // 0→°; 1→rad
       add_const(10, "_history");
       unsigned int pi_id = add_const(M_PI, "π");
       */
    funcs = makeList(sizeof(DefinedFunction));
    /*
       set_func_args(add_func("use_deg"), makeList(sizeof(char *)));
       set_func_args(add_func("use_rad"), makeList(sizeof(char *)));
       set_func_args(add_func("angle", LIST_CONV(char *, 1, { "x" })));
       set_func_args(add_func("set_history_size"), LIST_CONV(char *, 1, { "hsz" }));
       set_func_args(add_func("his"), LIST_CONV(char *, 1, { "n" }));
       set_func_body(set_func_args(add_func("deg2rad"), LIST_CONV(char *, 1, { "a_deg" })),
       LIST_CONV(Token, 5, ARG({
       { Const, { .id = pi_id } }, { Operator, { .op = OMul } },
       { Arg, { .id = 0 } }, { Operator, { .op = OTDiv } },
       { Number, { .number = 180 } }
       })));
       set_func_body(set_func_args(add_func("rad2deg"), LIST_CONV(char *, 1, { "a_rad" })),
       LIST_CONV(Token, 5, ARG({
       { Arg, { .id = 0 } }, { Operator, { .op = OTDiv } },
       { Const, { .id = pi_id } }, { Operator, { .op = OMul } },
       { Number, { .number = 180 } }
       })));
    // TODO builtin functions here
    */
}

void destroy_calc() {
    destroyList(vars);
    destroyList(consts);
    FOR_LIST_COUNTER(funcs, fn, DefinedFunction, func) {
        if (func->args != NULL)
            destroyList(func->args);
        if (!func->predef && func->body.tokens != NULL)
            destroyList(func->body.tokens);
    }
    destroyList(funcs);
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
    return getListLength(vars);
}

unsigned int count_const() {
    return getListLength(consts);
}

unsigned int count_func() {
    return getListLength(funcs);
}

unsigned int add_var(double init_val, char *name) {
    NamedValue nv = { name, init_val };
    /*
    FOR_LIST_COUNTER(vars, vidx, NamedValue, var)
        if (var->name == NULL) {
            *var = nv;
            return vidx;
        }
    */
    listAppend(vars, &nv);
    return getListLength(vars) - 1;
}

double get_var(unsigned int varid) {  // assumes that var with id varid already exists
    return ((NamedValue *)getListItemValue(vars, varid))->value;
}

char *get_var_name(unsigned int varid) {
    return ((NamedValue *)getListItemValue(vars, varid))->name;
}

void set_var(unsigned int varid, double value) {
    ((NamedValue *)getListItemValue(vars, varid))->value = value;
}

void remove_var(unsigned int varid) {
    // *(NamedValue *)getListItemValue(vars, varid) = (NamedValue){ NULL, 0 };
    listRemove(vars, varid);
}

unsigned int add_const(double value, char *name) {
    NamedValue nc = { name, value };
    /*
    FOR_LIST_COUNTER(consts, cidx, NamedValue, cnst)
        if (cnst->name == NULL) {
            *cnst = nv;
            return cidx;
        }
    */
    listAppend(consts, &nc);
    return getListLength(consts) - 1;
}

double get_const(unsigned int cid) {
    return ((NamedValue *)getListItemValue(consts, cid))->value;
}

char *get_const_name(unsigned int cid) {
    return ((NamedValue *)getListItemValue(consts, cid))->name;
}

void set_const(unsigned int cid, double val) {
    ((NamedValue *)getListItemValue(consts, cid))->value = val;
}

void remove_const(unsigned int cid) {
    // *(NamedValue *)getListItemValue(consts, cid) = (NamedValue){ NULL, 0 };
    listRemove(consts, cid);
}

unsigned int add_func(char *name) {
    DefinedFunction defun = { name, NULL, true, { NULL } };
    listAppend(funcs, &defun);
    return getListLength(funcs) - 1;
}

unsigned int set_func_args(unsigned int fid, struct list_head *args) {
    DefinedFunction *func = getListItemValue(funcs, fid);
    if (func->args != NULL)
        destroyList(func->args);
    func->args = args;
    return fid;
}

unsigned int set_func_body(unsigned int fid, struct list_head *body) {
    DefinedFunction *func = getListItemValue(funcs, fid);
    if (!func->predef && func->body.tokens != NULL)
        destroyList(func->body.tokens);
    func->body.tokens = body;
    func->predef = false;
    return fid;
}

unsigned int set_func_func(unsigned int fid, PredefFunc f) {
    DefinedFunction *func = getListItemValue(funcs, fid);
    if (!func->predef && func->body.tokens != NULL)
        destroyList(func->body.tokens);
    func->body.f = f;
    func->predef = true;
    return fid;
}

unsigned int get_func_argc(unsigned int fid) {
    return getListLength(((DefinedFunction *)getListItemValue(funcs, fid))->args);
}

char *get_func_arg_name(unsigned int fid, unsigned int arg_idx) {
    return *(char **)getListItemValue(get_func_args(fid), arg_idx);
}

struct list_head *get_func_args(unsigned int fid) {
    DefinedFunction *funcp = getListItemValue(funcs, fid);
    if (funcp->args == NULL)
        funcp->args = makeList(sizeof(char *));
    return funcp->args;
}

char *get_func_name(unsigned int fid) {
    return ((DefinedFunction *)getListItemValue(funcs, fid))->name;
}

struct list_head *eval_all_args(struct list_head *args) {
    struct list_head *res = makeList(sizeof(double));
    FOR_LIST_COUNTER(args, argn, list, argv) {
        double val = eval_expr(*argv, 0);
        listAppend(res, &val);
    }
    return res;
}

double call_func(unsigned int fid, struct list_head *argv) {
    DefinedFunction *func = getListItemValue(funcs, fid);
    if (func->predef)
        return func->body.f(argv);
    struct list_head *argval = eval_all_args(argv);
    struct list_head *body = listCopy(func->body.tokens);
    unsigned int len = getListLength(body);
    unsigned int idx = 0;
    for (Token *itmp = (Token *)getListItemValue(body, 0);
            idx < len;
            itmp = (Token *)getListNextItem(itmp)) {
        if (itmp->type == Arg) {
            itmp->type = Number;
            itmp->value.number = *(double *)getListItemValue(argval, itmp->value.id);
        }
        idx += 1;
    }
    double res = eval_expr(body, 0);
    destroyList(body);
    destroyList(argval);
    return res;
}

void remove_func(unsigned int fid) {
    DefinedFunction *func = getListItemValue(funcs, fid);
    if (func->args != NULL)
        destroyList(func->args);
    if (!func->predef && func->body.tokens != NULL)
        destroyList(func->body.tokens);
    listRemove(funcs, fid);
}

bool is_predefined_func(unsigned int fid) {
    return ((DefinedFunction *)getListItemValue(funcs, fid))->predef;
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

LValue eval_lvalue(struct list_head *tokens) {
    unsigned int id = GET_TOKEN(tokens, 0).value.id;
    if (getListLength(tokens) == 1)
        return (LValue){
            id,
                false
        };
    else {  // assume that tokens[1] is an OLCall
        /* from here goes the DEFUN */
        // DefinedFunction defun = { NULL, NULL, NULL };
        /*
           struct list_head *args = makeList(sizeof(char *));
           void *item = NULL;
           for (Token *iter = (Token *)getListItemValue(tokens, 2); ((Token *)getListNextItem(iter))->type != ORCall; iter = (Token *)getListNextItem(getListNextItem(iter)))
           listAppend(args, &item);
           */
        /* end of DEFUN */
        return (LValue){
            id, true
        };
    }
}

LValue eval_assign(LValue lvalue, struct list_head *rvalue) {
    if (lvalue.is_func) {
        set_func_body(lvalue.id, rvalue);
    } else {
        set_var(lvalue.id, eval_expr(rvalue, 0));
    }
    return lvalue;
}

double eval_funcall(unsigned int fid, struct list_head *args) {
    return call_func(fid, args);
}

double eval_expr(struct list_head *tokens, int operator_order) {
    unsigned int tlen = getListLength(tokens);
    if (tlen == 1)
        return eval_atom(GET_TOKEN(tokens, 0));
    int nested = 0;
    Token *otoken = NULL;
    unsigned int oindex = 0;
    bool par_ok = false;
    unsigned int indx = 0;
    for (Token *iter = getListItemValue(tokens, 0); indx < tlen; iter = (Token *)getListNextItem(iter)) {
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
                case OLCall:
                    if (!nested && operator_order == 5)
                    { otoken = iter; oindex = indx; }
                    // no break
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
        struct list_head *xpr = listSlice(tokens, 1, tlen - 1);
        double res = eval_expr(xpr, operator_order);
        destroyListHeader(xpr);
        return res;
    }
    if (otoken == NULL)  // no such token
        return eval_expr(tokens, operator_order + 1);
    if (otoken->type == UOperator) {
        struct list_head *xpr = listSlice(tokens, oindex + 1, tlen);
        double res = eval_uoperation(otoken->value.uop, eval_expr(xpr, 0));
        destroyListHeader(xpr);
        return res;
    } else if (otoken->type == Operator && otoken->value.op == OLCall) {
        struct list_head *args = makeList(sizeof(list));
        unsigned int nesting = 0;
        unsigned int sindex = oindex;
        unsigned int iindex = oindex + 1;
        struct list_head *curarg;
        FOR_LIST(tokens, iindex, Token, iter)
            if (iter->type == Operator)
                switch (iter->value.op) {
                    case OLCall:
                        nesting += 1;
                        break;
                    case ORCall:
                        if (nesting)
                            nesting -= 1;
                        else {
                            curarg = listSlice(tokens, sindex + 1, iindex);
                            listAppend(args, &curarg);
                            // destroyListHeader(curarg);
                            goto eval_f;
                        }
                        break;
                    case OComma:
                        if (!nesting) {
                            curarg = listSlice(tokens, sindex + 1, iindex);
                            listAppend(args, &curarg);
                            // destroyListHeader(curarg);
                            sindex = iindex;
                        }
                        break;
                    default:
                        break;
                }
eval_f:;
       double res = eval_funcall(GET_TOKEN(tokens, oindex - 1).value.id, args);
       FOR_LIST_COUNTER(args, argn, list, arg)
           destroyListHeader(*arg);
       destroyList(args);
       return res;
    }
    if (otoken->type == Operator && otoken->value.op == OAssign) {
        struct list_head *lval = listSlice(tokens, 0, oindex);
        struct list_head *rval = listDetached(LIST_TAIL(tokens, oindex + 1));
        LValue res = eval_assign(eval_lvalue(lval), rval);
        destroyListHeader(lval);
        return res.is_func ? 0 : get_var(res.id);
    } else {
        struct list_head *op1 = listSlice(tokens, 0, oindex);
        struct list_head *op2 = listSlice(tokens, oindex + 1, tlen);
        double res = eval_operation(otoken->value.op, eval_expr(op1, 0), eval_expr(op2, 0));
        destroyListHeader(op1);
        destroyListHeader(op2);
        return res;
    }
}

