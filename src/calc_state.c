#include "calc_state.h"

static void cs_call_cb(CalcState *cs) {
    if (!(cs->no_cb) && cs->callback != NULL)
        cs->callback(cs);
}

static void cs_set_nocb(CalcState *cs, bool nocb) {  // for multiple modifications at once
    cs->no_cb = nocb;
}

static bool cs_inside_defun(CalcState *cs) {
    return cs->scope.offset;
}

static OperatorType _bid_to_operator(TokenItemId tii) {  // just in case
    return tii - TIIPlus;
}

static TokenItemId _operator_to_bid(OperatorType op) {  // idem
    return op + TIIPlus;
}

static UOperatorType _tii_to_uoperator(TokenItemId tii) {
    return tii - TIIUPlus;
}

/*
static TokenItemId _uoperator_to_tii(UOperatorType uop) {
    return uop + TIIUPlus;
}
*/

static CalcInputType _tii_to_cit(TokenItemId tii) {
    return tii - TIINumber + ITNumber;
}

static enum token_type _cit_to_tt(CalcInputType cit) {
    switch (cit) {
        case ITNumber:
            return Number;
        case ITArg:
            return Arg;
        default:  // fails for ITNone
            return cit - ITVar;
    }
}

#define CALC_WITH_NO_CB(calcstate, body) { cs_set_nocb(calcstate, true); body cs_set_nocb(calcstate, false); }

char *getButtonText(TokenItemId tii) {
    static char *chars[] = { "<-", "+", "-", "*", "/", "//", "%", "**", "(", ")", "=", "f(", "f,", "f)",
        "+arg", "num", "var", "const", "func", "arg", "+x", "-x", "->", "->c", "->n" };  // may be tmp
    return chars[tii];
}

char *cs_get_token_text(CalcState *cs, Token token) {
    static char buffer[20];
    switch (token.type) {
        case Var:
            return get_var_name(token.value.id);
        case Const:
            return get_const_name(token.value.id);
        case Func:
            return get_func_name(token.value.id);
        case Number:
            ftoa(token.value.number, buffer, sizeof(buffer));
            return buffer;
        case Operator:
            switch (token.value.op) {
                case OLCall:
                    return "(";
                case ORCall:
                    return ")";
                case OComma:
                    return ",";
                default:
                    return getButtonText(_operator_to_bid(token.value.op));
            }
        case UOperator:
            if (token.value.uop == UPlus)
                return "+";
            else
                return "-";
        case Arg:
            {
                unsigned int len = cs->fcalls->length;
                return get_func_arg_name(
                        (len
                         ? ((FuncallMark *)cs->fcalls->first->data)->fid
                         : cs->scope.fid),
                        token.value.id);
            }
        default:
            return "error";
    }
}

static void cs_reset(CalcState *cs) {
    cs->exp = TEValue;
    cs->nesting = 0;
    cs->str = malloc(CALC_STR_BLOCK_SIZE);
    *(cs->str) = 0;
    cs->block_size = CALC_STR_BLOCK_SIZE;
    cs->str_len = 1;  // null-terminated string
    cs->no_cb = false;
    cs->cit = ITNone;
}

CalcState *cs_create() {
    CalcState *cs = malloc(sizeof(CalcState));
    cs->expr = makeList(sizeof(Token));
    cs->callback = NULL;
    cs->eval_cb = NULL;
    cs->fcalls = makeList(sizeof(FuncallMark));
    cs->names = makeList(sizeof(NewNameMark));
    cs->symbols = makeList(sizeof(char *));
    cs->argnames = 0;
    cs->scope = (DefunMark){ 0, 0, 0 };
    cs->history_size = 0;
    cs_reset(cs);
    return cs;
}

double cs_eval(CalcState *cs) {
    // apply names here
    listClear(cs->names);
    double res = eval_expr(cs_get_expr(cs), 0);
    if (cs->eval_cb != NULL)
        cs->eval_cb(cs, res);
    return res;
}

void cs_clear(CalcState *cs) {
    listClear(cs->expr);
    listClear(cs->fcalls);
    cs->scope.offset = 0;
    free(cs->str);
    cs_reset(cs);
}

void cs_set_cb(CalcState *cs, CalcStateChangedCb cb, CalcEvalCb ecb) {
    cs->callback = cb;
    cs->eval_cb = ecb;
}

#define MOD_FLAGS(lvalue, addf, remf) { lvalue |= addf; lvalue &= ~(remf); }

void cs_modify_expect(CalcState *cs, Token *new_tok_p) {
    // defun check: ORCall, prev is Arg, cs.defun.offset == 0
    // set var check: Var, either the first token or prev is OComma or OLCall or OLp
    cs->exp &= ~(TEFRCall | TEFComma | TECloseParen | TEFArgs);
    unsigned int len = cs->expr->length;
    switch (new_tok_p->type) {
        case Arg:
            if (cs->scope.offset == 0)
                cs->exp = TEFRCall | TEFComma;
            else
                MOD_FLAGS(cs->exp, TEBinaryOperator, TEValue | TEFuncall);
            break;
        case Var:
            {
                Token *tok;
            if (len == 1 || (len > 1 && ((tok = GET_PTOKEN(cs->expr, len - 2))->type == Operator)
                    && (tok->value.op == OComma || tok->value.op == OLCall || tok->value.op == OLp)))
                cs->exp |= TEAssign;
            }  // no break
        case Const:
        case Number:
            MOD_FLAGS(cs->exp, TEBinaryOperator, TEValue | TEFuncall);
            // special for funcall
            // special for defun
            break;
        case Func:
            MOD_FLAGS(cs->exp, TEFuncall, TEValue | TEBinaryOperator | TEAssign);
            break;
        case Operator:
            {
                OperatorType op = new_tok_p->value.op;
                switch (op) {
                    case ORCall:
                        // may trigger TEAssign for the defun
                        {
                            Token *prev = GET_PTOKEN(cs->expr, len - 2);
                            if (prev->type == Arg
                                    && cs->scope.offset == 0)
                                cs->exp |= TEAssign;
                            else if (prev->type == Operator
                                    && prev->value.op == OLCall)
                                MOD_FLAGS(cs->exp,
                                        TEBinaryOperator | TEAssign,
                                        TEValue);
                        }  // no break
                    case OLp:
                    case ORp:
                        // do nothing, not the default
                        break;
                    case OAssign:
                        // TEFArgs is set below
                        MOD_FLAGS(cs->exp, TEValue, TEBinaryOperator | TEAssign);
                        break;
                    case OLCall:
                        // check if defun is possible - is 2nd token
                        if (len == 2)
                            cs->exp |= TEFArgs;
                        MOD_FLAGS(cs->exp,
                                TEValue | TEFRCall,
                                TEBinaryOperator | TEAssign | TEFuncall);
                        break;
                    case OComma:
                        // special if prev is Arg and no defun
                        if (GET_PTOKEN(cs->expr, len - 2)->type == Arg
                         && cs->scope.offset == 0) {
                            cs->exp = TEFArgs;
                            break;
                        }
                        // no break
                    default:
                        MOD_FLAGS(cs->exp, TEValue, TEBinaryOperator | TEAssign | TEFuncall);
                        break;
                }
            }
            break;
        default:
            // UOp -> as-is
            break;
    }
    FuncallMark *fmp;
    if ((new_tok_p->type != Operator || (new_tok_p->type == Operator &&
        (new_tok_p->value.op == ORp || new_tok_p->value.op == ORCall)))
     && new_tok_p->type != UOperator
     && cs->fcalls->length
     && cs->nesting - 1 == (fmp = ((FuncallMark *)cs->fcalls->first->data))->nesting_level
     && cs->exp & TEBinaryOperator) {
        MOD_FLAGS(cs->exp, TEFRCall | TEFComma, TENone);
    } else if (
        new_tok_p->type != Operator
     && new_tok_p->type != UOperator
     && cs->nesting
     && (!cs->fcalls->length
       || cs->nesting - 1 != ((FuncallMark *)cs->fcalls->first->data)->nesting_level)) {
        MOD_FLAGS(cs->exp, TECloseParen, TENone);
    }
    if (cs->scope.offset && cs->exp & TEValue)
        cs->exp |= TEFArgs;
}

bool cs_show_item(CalcState *cs, TokenItemId tii) {
    switch (tii) {
        case TIIBackspace:
            return cs->expr->length != 0;
        case TIIPlus: case TIIMinus: case TIIMultiply: case TIITrueDiv:
        case TIIFloorDiv: case TIIModulo: case TIIPower:
            return (cs->exp & TEBinaryOperator) == TEBinaryOperator;
        case TIILParen: case TIIVar: case TIINumber:
        case TIIFunction: case TIIUPlus: case TIIUMinus:
            return (cs->exp & TEValue) == TEValue;
        case TIIConst:
            return (cs->exp & TEValue) && (count_const());
        case TIIRParen:
            return (cs->exp & TECloseParen) == TECloseParen;
        case TIIAssign:
            return (cs->exp & TEAssign) == TEAssign;
        case TIILFuncall:
            return (cs->exp & TEFuncall) == TEFuncall;
        case TIIComma:
            return (cs->exp & TEFComma) == TEFComma;
        case TIIRFuncall:
            return (cs->exp & TEFRCall) == TEFRCall;
        case TIIAddArg:
            return (cs->exp & TEFArgs) && (cs->scope.offset == 0);
        case TIIArg:
            return (cs->exp & TEFArgs) && (cs->scope.offset != 0);
        case TIIReturn: case TIISaveToConst: case TIISaveToNamedConst:
            return cs->expr->length
                && !(cs->nesting)
                && (cs->exp & TEBinaryOperator) == TEBinaryOperator;
    }
    return false;
}

void cs_add_item(CalcState *cs, Token new_tok) {
    static unsigned int fid;
    if (new_tok.type == Operator)
        switch (new_tok.value.op) {  // special actions for special types of Tokens
            case OLCall:
                {
                    static FuncallMark fm;
                    fm.nesting_level = cs->nesting;
                    fm.fid = GET_PTOKEN(cs->expr, cs->expr->length - 1)->value.id;
                    fm.arg_idx = 0;
                    fid = fm.fid;  // set fid here in case of a defun
                    listPush(cs->fcalls, &fm);
                }  // no break
            case OLp:
                cs->nesting += 1;
                break;
            case OComma:
                if (cs->scope.nesting_level >= cs->nesting)
                    cs->scope.offset = 0;
                ((FuncallMark *)cs->fcalls->first->data)->arg_idx += 1;
                break;
            case ORCall:
                if (cs->scope.nesting_level >= cs->nesting)
                    cs->scope.offset = 0;
                listRemove(cs->fcalls, 0);  // no break
            case ORp:
                cs->nesting -= 1;
                break;
            case OAssign:
                {
                    unsigned int len = cs->expr->length;
                    if (!len)
                        break;
                    Token *prev = GET_PTOKEN(cs->expr, len - 1);
                    if (prev->type == Operator && prev->value.op == ORCall) {  // it's a defun
                        // fid should be set already
                        cs->scope.nesting_level = cs->nesting;
                        cs->scope.offset = len + 1;
                        cs->scope.fid = fid;
                    }
                }
                break;
            default:
                break;
        }
    listAppend(cs->expr, &new_tok);
    cs_modify_expect(cs, &new_tok);
    char *s = cs_get_token_text(cs, new_tok);
    size_t slen = strlen(s);
    size_t newbs = cs->block_size;
    cs->str_len += slen;
    while (newbs < cs->str_len)
        newbs += CALC_STR_BLOCK_SIZE;
    if (newbs != cs->block_size) {
        cs->str = realloc(cs->str, newbs);
        cs->block_size = newbs;
    }
    strcat(cs->str, s);
    cs->cit = ITNone;
    cs_call_cb(cs);
}

void cs_rem_item(CalcState *cs) {
    struct list_head *expr = cs->expr;
    unsigned int len = expr->length - 1;
    Token *t = GET_PTOKEN(expr, len);
    size_t slen = strlen(cs_get_token_text(cs, *t));  // value may be removed
    if (t->type == Operator)
        switch (t->value.op) {
            case OLCall:
                listRemove(cs->fcalls, 0);  // no break
            case OLp:
                cs->nesting -= 1;
                break;
            case OComma:
                ((FuncallMark *)cs->fcalls->first->data)->arg_idx -= 1;
                break;
            case ORCall:
                {
                    static FuncallMark fm;
                    fm.nesting_level = cs->nesting + 1;
                    FOR_LIST_COUNTER(expr, tindex, Token, token) {
                        if (token->type == Func) {
                            Token *ntoken = getListNextItem(token);
                            if (ntoken->type == Operator && ntoken->value.op == OLCall) {
                                fm.fid = token->value.id;
                                fm.arg_idx = 0;
                            }
                        } else if (token->type == Operator && token->value.op == OComma)
                            fm.arg_idx += 1;
                    }
                    listPush(cs->fcalls, &fm);
                }  // no break
            case ORp:
                cs->nesting += 1;
                break;
            default:
                break;
        }
    else if (t->type != Number && t->type != UOperator
            && cs->names->length
            && (((NewNameMark *)cs->names->first)->offset == len))
        // automaticaly remove the unneeded symbols here
        cs_remove_unneeded(cs);
    size_t newbs = cs->block_size;
    cs->str_len -= slen;
    while (newbs > CALC_STR_MAX_TAKEN_LEN && cs->str_len <= (newbs - CALC_STR_BLOCK_SIZE))
        newbs -= CALC_STR_BLOCK_SIZE;
    if (newbs != cs->block_size) {
        cs->str = realloc(cs->str, newbs);
        cs->block_size = newbs;
    }
    cs->str[cs->str_len - 1] = 0;
    listRemove(expr, len);
    cs->exp = TEValue;
    // re-expect
    FOR_LIST_COUNTER(cs->expr, idx, Token, tok)
        cs_modify_expect(cs, tok);
    // callback
    cs_call_cb(cs);
}

static unsigned int get_dig_num(unsigned int i) {
    unsigned int a = 10;
    unsigned int b = 1;
    for (;a < i; b++)
        a *= 10;
    return b;
}

CalcInputType cs_interact(CalcState *cs, TokenItemId tii) {
    switch (tii) {
        case TIIPlus: case TIIMinus: case TIIMultiply: case TIITrueDiv:
        case TIIFloorDiv: case TIIModulo: case TIIPower: case TIILParen:
        case TIIRParen: case TIIAssign: case TIILFuncall: case TIIComma:
        case TIIRFuncall:  // don't want to use IFs of ranges (gcc/clang ext) to be able to change the order
            cs_add_item(cs, (Token){ Operator, { .op = _bid_to_operator(tii) } });
            break;
        case TIIUPlus: case TIIUMinus:
            cs_add_item(cs, (Token){ UOperator, { .uop = _tii_to_uoperator(tii) } });
            break;
        case TIINumber: case TIIVar: case TIIConst: case TIIFunction: case TIIArg:
            cs->cit = _tii_to_cit(tii);
            return cs->cit;
        case TIIBackspace:
            cs_rem_item(cs);
            break;
        case TIIReturn:
            cs_eval(cs);
            break;
        case TIISaveToConst:
            {
                size_t sl = 2 + get_dig_num(++cs->history_size);
                char *cn = malloc(sl);
                snprintf(cn, sl, "$%d", cs->history_size);
                add_const(cs_eval(cs), cn);
            }
            break;
        case TIISaveToNamedConst:
            cs->cit = ITNC;
            return ITNC;
        case TIIAddArg:
            cs_interact(cs, TIIArg);
            cs_input_newarg(cs);
            break;
        default:
            break;
    }
    return ITNone;
}

void cs_input_number(CalcState *cs, double num) {
    if (cs->cit == ITNumber)
        cs_add_item(cs, (Token){ Number, (TokenValue){ .number = num } });
}

void cs_input_id(CalcState *cs, unsigned int id) {
    if (cs->cit >= ITVar)
        cs_add_item(cs, (Token){ _cit_to_tt(cs->cit), { .id = id } });
}

/*
 * Add the name of the symbol to the symbol table.
 * Use if the name is located in the heap,
 * as these names should be free()'d.
 * free() is called for all names in the symbol table
 * when the CalcState is destroyed (see cs_destroy()).
 * The type of the symbol is determined according to
 * the CurrentInputType (see cs_get_input_type()).
 */
void cs_input_newid(CalcState *cs, unsigned int id) {
    /*
    if ((cs->cit < ITVar) && (cs->cit != ITNC))
        return;
        */
    switch (cs->cit) {
        case ITVar:
            cs_add_symbol(cs, get_var_name(id));
            break;
        case ITFunc:
            cs_add_symbol(cs, get_func_name(id));
            break;
        case ITNC:
            cs_add_symbol(cs, get_const_name(id));
            return cs_input_id(cs, id);
        default:
            return;
    }
    NewNameMark nnm = (NewNameMark){ cs->expr->length, cs->cit, id };
    listPush(cs->names, &nnm);
    cs_input_id(cs, id);
}

static char **cs_get_argname(CalcState *cs, unsigned int idx) {
    for (; cs->argnames <= idx; cs->argnames++) {
        size_t sn = get_dig_num(cs->argnames + 1) + 2;
        char *buf = malloc(sn);
        snprintf(buf, sn, "p%d", cs->argnames + 1);
        listInsert(cs->symbols, cs->argnames, &buf);
    }
    return LIST_DATA(char *, cs->symbols, idx);
}

void cs_input_newarg(CalcState *cs) {
    if (cs->cit == ITArg) {
        FuncallMark *fm = LIST_DATA(FuncallMark, cs->fcalls, 0);
        listAppend(get_func_args(fm->fid),
            cs_get_argname(cs, fm->arg_idx));
        cs_input_id(cs, fm->arg_idx);
    }
}

void cs_add_symbol(CalcState *cs, char *sym) {
    listAppend(cs->symbols, &sym);
}

void cs_remove_symbol(CalcState *cs, char *sym) {
    listRemove(cs->symbols, getListItemIndex(cs->symbols, &sym));
    // if sym is not found, nothing happens anyway
}

void cs_remove_unneeded(CalcState *cs) {
    NewNameMark nnm = LIST_REF(NewNameMark, cs->names, 0);
    listRemove(cs->names, 0);
    switch (nnm.type) {
        case ITVar:
            cs_remove_symbol(cs, get_var_name(nnm.id));
            remove_var(nnm.id);
            break;
        case ITFunc:
            cs_remove_symbol(cs, get_func_name(nnm.id));
            remove_func(nnm.id);
            break;
            /*
        case ITArg:
            break;
            */
        default:
            break;
    }
}

CalcInputType cs_get_input_type(CalcState *cs) {
    return cs->cit;
}

void cs_destroy(CalcState *cs) {
    destroyList(cs->expr);
    free(cs->str);
    destroyList(cs->fcalls);
    destroyList(cs->names);
    FOR_LIST_COUNTER(cs->symbols, index, char *, sym)
        free(*sym);
    destroyList(cs->symbols);
    free(cs);
}

char *cs_curr_str(CalcState *cs) {
    return cs->str;
}

unsigned int cs_get_func_id(CalcState *cs) {
    return cs->scope.fid;
}

struct list_head *cs_get_expr(CalcState *cs) {
    return cs->expr;
}

