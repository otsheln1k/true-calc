#include <stdio.h>
#include <math.h>

#include "calc_state.h"
#include "eval.h"
#include "list.h"
#include "ftoa.h"


/*
 * Add ‘name’ field to ‘struct new_name_mark’?
 */


/* KNOWN BUGS */

/*
 * Currently, inline defuns are allowed but don’t work
 * This is a bug (in case you haven’t noticed)
 */

/*
 * Function redefinition also seems to be allowed
 */

/*
 * Calling an undefined (not-yet-defined) function
 * should be undefined/forgiven
 */

/*
 * true-calc crashes when evaluating a zero-argument
 * function; it’s a bug
 */

/*
 * Recursion is allowed but can’t be evaluated because
 * of the lack of lazy or conditional evaluation
 * facitilies
 */

/*
 * Not sure but it seems to me that there’s a bug
 * in ‘cs_get_token_text’ (case Arg): we don’t need
 * argument name of an already defined func
 */

/*
 * More effective list access?
 * Keep the list tail in calc state
 * Thus, we could add tokens in O(1) (though remove in O(n))
 *
 * Even better, we could push tokens to head; then reverse
 * and evaluate
 *
 * For now, consider inefficient token list storage/access
 * as a bug
 */


/* NOT-YET-KNOWN BUGS */

void cs_call_cb(struct calc_state *cs) {
    if (!(cs->no_cb) && cs->callback != NULL)
        cs->callback(cs);
}

void cs_set_nocb(struct calc_state *cs, int nocb) {
    cs->no_cb = nocb;
}

static unsigned int digit_count(unsigned int i) {
    unsigned int a = 10;
    unsigned int b = 1;
    for (;a < i; b++)
        a *= 10;
    return b;
}

static char *alloc_string(char *str) {
    char *s1 = malloc(strlen(str));
    strcpy(s1, str);
    return s1;
}

static char *make_prefixed_number_string(const char *prefix,
                                         unsigned int number)
{
    size_t len = strlen(prefix) + digit_count(number) + 1;
    char *str = malloc(len * sizeof(*str));
    sprintf(str, "%s%u", prefix, number);
    return str;
}

static char **cs_get_argname(struct calc_state *cs, unsigned int idx) {
    char *buf;
    
    for (; cs->argnames <= idx; cs->argnames++) {
        buf = make_prefixed_number_string("p", cs->argnames + 1);
        listInsert(cs->symbols, cs->argnames, &buf);
    }
    return LIST_DATA(char *, cs->symbols, idx);
}


const char *getButtonText(enum token_item_id tii) {
    static const char *button_strings[] = {
        "<-",   "+",    "-",    "*",    "/",    "//",   "%",    "**",
        "(",    ")",    "=",    "f(",   "f,",   "f)",   "+arg", "num",
        "var",  "const","func", "arg",  "+x",   "-x",   "->",   "->c",
        "->n",
    };
    return button_strings[tii];
}

const char *cs_get_token_text(struct calc_state *cs, struct token *token) {
    static char buffer[20];

    switch (token->type) {
    case Var:
        return get_var_name(token->value.id);
    case Const:
        return get_const_name(token->value.id);
    case Func:
        return get_func_name(token->value.id);
    case Number:
        ftoa(token->value.number, buffer, sizeof(buffer));
        return buffer;
    case Operator:
        switch (token->value.op) {
        case OLCall:
            return "(";
        case ORCall:
            return ")";
        case OComma:
            return ",";
        default:
            return getButtonText(OPTYPE_TO_TII(token->value.op));
        }
    case UOperator:
        if (token->value.uop == UPlus)
            return "+";
        else
            return "-";
    case Arg:
        // TODO: might be a bug
        return get_func_arg_name(
            (cs->fcalls->length
             ? TOP_FUNCALL(cs)->fid
             : cs->scope.fid),
            token->value.id);
    default:
        return "error";
    }
}


static void cs_reset(struct calc_state *cs) {
    cs->exp = TEValue;
    cs->nesting = 0;
    cs->str = malloc(CALC_STR_BLOCK_SIZE);
    cs->str[0] = 0;
    cs->str_sz = CALC_STR_BLOCK_SIZE;
    cs->str_len = 1;  // null-terminated string
    cs->no_cb = 0;
    cs->cit = ITNone;
}

struct calc_state *cs_create() {
    struct calc_state *cs = malloc(sizeof(*cs));
    cs->expr = makeList(sizeof(struct token));
    cs->callback = NULL;
    cs->eval_cb = NULL;
    cs->fcalls = makeList(sizeof(struct funcall_mark));
    cs->names = makeList(sizeof(struct new_name_mark));
    cs->symbols = makeList(sizeof(char *));
    cs->argnames = 0;
    cs->scope = (struct defun_mark){ 0, 0, 0 };
    cs->history_size = 0;
    cs_reset(cs);
    return cs;
}

double cs_eval(struct calc_state *cs) {
    // ???
    // apply names here
    listClear(cs->names);
    double res = eval_expr(cs_get_expr(cs), 0);
    if (cs->eval_cb != NULL)
        cs->eval_cb(cs, res);
    return res;
}

void cs_clear(struct calc_state *cs) {
    listClear(cs->expr);
    listClear(cs->fcalls);
    cs->scope.offset = 0;
    free(cs->str);
    cs_reset(cs);
}

void cs_destroy(struct calc_state *cs) {
    destroyList(cs->expr);
    free(cs->str);
    destroyList(cs->fcalls);
    destroyList(cs->names);
    FOR_LIST_COUNTER(cs->symbols, index, char *, sym)
        free(*sym);
    destroyList(cs->symbols);
    free(cs);
}


void cs_set_cb(struct calc_state *cs,
               calc_state_changed_cb cb,
               calc_eval_cb ecb) {
    cs->callback = cb;
    cs->eval_cb = ecb;
}

#define UPDATE_FLAGS(lvalue, addf, remf) { lvalue |= addf; lvalue &= ~(remf); }


/*
 * Most bugs can be fixed by making certain token sequencies
 * firbidden
 */
void cs_update_expect(struct calc_state *cs, struct token *new_tok_p) {
    // defun check: ORCall, prev is Arg, cs.defun.offset == 0
    // set var check: Var, either the first token or prev is OComma or OLCall or OLp
    unsigned int len = cs->expr->length;

    cs->exp &= ~(TEFRCall | TEFComma | TECloseParen | TEFArgs);
    
    switch (new_tok_p->type) {
    case Arg:
        if (cs->scope.offset == 0)
            cs->exp = TEFRCall | TEFComma;
        else
            UPDATE_FLAGS(cs->exp,
                         TEBinaryOperator,
                         TEValue | TEFuncall);
        break;
    case Var: {
        struct token *tok;
        if (len == 1
            || (len > 1
                && ((tok = GET_PTOKEN(cs->expr, len - 2))->type == Operator)
                && (tok->value.op == OComma
                    || tok->value.op == OLCall
                    || tok->value.op == OLp)))
            cs->exp |= TEAssign;
        // fall through
    }
    case Const:
    case Number:
        UPDATE_FLAGS(cs->exp,
                     TEBinaryOperator,
                     TEValue | TEFuncall);
        // ???
        // special for funcall
        // special for defun
        break;
    case Func:
        UPDATE_FLAGS(cs->exp,
                     TEFuncall,
                     TEValue | TEBinaryOperator | TEAssign);
        break;
    case Operator:
        switch (new_tok_p->value.op) {
        case ORCall: {
            // may trigger TEAssign for the defun
            struct token *prev = GET_PTOKEN(cs->expr, len - 2);

            /* this one seems to be an error:
             * f ( _)_ [can assign]
             * f ( a1, a2 _)_ [can assign]
             * v1 + f ( _)_ [cannot assign here]
             */
            if (prev->type == Arg
                && cs->scope.offset == 0)
                cs->exp |= TEAssign;
            else if (prev->type == Operator
                     && prev->value.op == OLCall)
                UPDATE_FLAGS(cs->exp,
                             TEBinaryOperator | TEAssign,
                             TEValue);
        }  // no break
        case OLp:
        case ORp:
            // do nothing, not the default
            break;
        case OAssign:
            // TEFArgs is set below
            UPDATE_FLAGS(cs->exp, TEValue, TEBinaryOperator | TEAssign);
            break;
        case OLCall:
            // check if defun is possible - is 2nd token
            if (len == 2)
                cs->exp |= TEFArgs;
            UPDATE_FLAGS(cs->exp,
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
            UPDATE_FLAGS(cs->exp, TEValue, TEBinaryOperator | TEAssign | TEFuncall);
            break;
        }
        break;
    case UOperator:
        // UOp -> as-is
    default:
        break;
    }

    if ((new_tok_p->type != Operator
         || (new_tok_p->type == Operator &&
             (new_tok_p->value.op == ORp
              || new_tok_p->value.op == ORCall)))
        && new_tok_p->type != UOperator
        && cs->fcalls->length
        && cs->nesting - 1 == TOP_FUNCALL(cs)->nesting_level
        && cs->exp & TEBinaryOperator) {
        cs->exp |= TEFRCall | TEFComma;
    } else if (new_tok_p->type != Operator
               && new_tok_p->type != UOperator
               && cs->nesting
               && (!cs->fcalls->length
                   || (cs->nesting - 1) !=
                   TOP_FUNCALL(cs)->nesting_level)) {
        cs->exp |= TECloseParen;
    }

    if (cs->scope.offset != 0 && cs->exp & TEValue)
        cs->exp |= TEFArgs;
}

int cs_show_item(struct calc_state *cs, enum token_item_id tii) {
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
    return 0;
}


/* ADD/REMOVE TOKENS */

void cs_add_item(struct calc_state *cs, struct token new_tok) {
    if (new_tok.type == Operator) {
        // special actions for special types of Tokens
        switch (new_tok.value.op) {
        case OLCall: {
            struct funcall_mark fm;
            fm.nesting_level = cs->nesting;
            fm.fid = GET_PTOKEN(cs->expr, cs->expr->length - 1)->value.id;
            fm.arg_idx = 0;
            // set fid here in case of a defun
            cs->prev_fid = fm.fid;
            listPush(cs->fcalls, &fm);
            // fall through
        }
        case OLp:
            cs->nesting += 1;
            break;
        case OComma:
            TOP_FUNCALL(cs)->arg_idx += 1;
            break;
        case ORCall:
            listRemove(cs->fcalls, 0);
            // fall through
        case ORp:
            cs->nesting -= 1;
            break;
        case OAssign: {
            unsigned int len = cs->expr->length;
            struct token *prev = GET_PTOKEN(cs->expr, len - 1);
            if (prev->type == Operator && prev->value.op == ORCall) {
                // it's a defun
                // prev_fid should be set already
                cs->scope.nesting_level = cs->nesting;
                cs->scope.offset = len + 1;
                cs->scope.fid = cs->prev_fid;
            }
            break;
        }
        default:
            break;
        }
    }
    listAppend(cs->expr, &new_tok);
    cs_update_expect(cs, &new_tok);
    const char *s = cs_get_token_text(cs, &new_tok);
    size_t slen = strlen(s);
    size_t newbs = cs->str_sz;
    cs->str_len += slen;
    newbs = (cs->str_len + CALC_STR_BLOCK_SIZE - 1) / CALC_STR_BLOCK_SIZE;
    if (newbs > cs->str_sz) {
        cs->str = realloc(cs->str, newbs);
        cs->str_sz = newbs;
    }
    strcat(cs->str, s);
    cs->cit = ITNone;
    cs_call_cb(cs);
}

void cs_rem_item(struct calc_state *cs) {
    // note: this is the *new* length
    unsigned int len = cs->expr->length - 1;
    struct token *t = GET_PTOKEN(cs->expr, len);

    if (t->type == Operator) {
        switch (t->value.op) {
        case OLCall:
            listRemove(cs->fcalls, 0);
            // fall through
        case OLp:
            cs->nesting -= 1;
            break;
        case OComma:
            TOP_FUNCALL(cs)->arg_idx -= 1;
            break;
        case ORCall: {
            struct funcall_mark fm;
            fm.nesting_level = cs->nesting + 1;
            FOR_LIST_COUNTER(cs->expr, tindex, struct token, token) {
                if (token->type == Func) {
                    struct token *ntoken = getListNextItem(token);
                    if (ntoken->type == Operator && ntoken->value.op == OLCall) {
                        fm.fid = token->value.id;
                        fm.arg_idx = 0;
                    }
                } else if (token->type == Operator && token->value.op == OComma)
                    fm.arg_idx += 1;
            }
            listPush(cs->fcalls, &fm);
            // fall through
        }
        case ORp:
            cs->nesting += 1;
            break;
        default:
            break;
        }
    }
    else if (t->type != Number && t->type != UOperator
             && cs->names->length
             && (TOP_NEW_NAME(cs)->offset == len))
        // automaticaly remove the unneeded symbols here
       cs_remove_unneeded(cs);
    cs->str[cs->str_len - 1] = 0;
    listRemove(cs->expr, len);
    cs->exp = TEValue;
    // re-expect
    FOR_LIST_COUNTER(cs->expr, idx, struct token, tok)
        cs_update_expect(cs, tok);
    // callback
    cs_call_cb(cs);
}


/* INTERACTIVE */

enum calc_input_type cs_interact(struct calc_state *cs,
                                 enum token_item_id tii) {
    switch (tii) {
    case TIIPlus: case TIIMinus: case TIIMultiply: case TIITrueDiv:
    case TIIFloorDiv: case TIIModulo: case TIIPower: case TIILParen:
    case TIIRParen: case TIIAssign: case TIILFuncall: case TIIComma:
    case TIIRFuncall:
        cs_add_item(cs, (struct token){
                Operator, { .op = TII_TO_OPTYPE(tii) }});
        break;
    case TIIUPlus: case TIIUMinus:
        cs_add_item(cs, (struct token){
                UOperator, { .uop = TII_TO_UOPTYPE(tii) }});
        break;
    case TIINumber: case TIIVar: case TIIConst: case TIIFunction: case TIIArg:
        cs->cit = TII_TO_CIT(tii);
        return cs->cit;
    case TIIBackspace:
        cs_rem_item(cs);
        break;
    case TIIReturn:
        cs_eval(cs);
        break;
    case TIISaveToConst:
        add_const(cs_eval(cs),
                  make_prefixed_number_string(
                      "$", ++cs->history_size));
        break;
    case TIISaveToNamedConst:
        cs->cit = ITNC;
        return ITNC;
    case TIIAddArg:
        cs->cit = ITArg;
        cs_input_newarg(cs);
        break;
    default:
        break;
    }
    return ITNone;
}

void cs_input_number(struct calc_state *cs, double num) {
    if (cs->cit == ITNumber)
        cs_add_item(cs, (struct token){
                Number, (TokenValue){ .number = num }});
}

void cs_input_id(struct calc_state *cs, unsigned int id) {
    /* if (cs->cit >= ITVar) */
    cs_add_item(cs, (struct token){
            CIT_TO_TOKEN_TYPE(cs->cit), { .id = id }});
}

/*
 * Add the name of the symbol to the symbol table, then
 * call this function.
 * Use if the name is located in the heap and should be
 * free()’d if the corresponding symbol is gone.
 * free() is called for all names in the symbol table
 * when the calc state is destroyed (see cs_destroy()).
 * The type of the symbol is determined according to
 * the CurrentInputType (see cs_get_input_type()).
 */
/* void cs_input_newid(struct calc_state *cs, unsigned int id) { */
unsigned int cs_input_newid(struct calc_state *cs, char *name) {
    unsigned int id;
    
    switch (cs->cit) {
        case ITVar:
            id = add_var(0, cs_add_symbol(cs, name));
            /* cs_add_symbol(cs, get_var_name(id)); */
            break;
        case ITFunc:
            id = add_func(cs_add_symbol(cs, name));
            /* cs_add_symbol(cs, get_func_name(id)); */
            break;
        case ITNC:
            return cs_add_const(cs, cs_eval(cs), name);
            /* cs_add_symbol(cs, get_const_name(id)); */
        default:
            /* never reached */
            return 0;
    }

    struct new_name_mark nnm = (struct new_name_mark){
        cs->expr->length, cs->cit, id};
    listPush(cs->names, &nnm);
    cs_input_id(cs, id);
    return id;
}

void cs_input_newarg(struct calc_state *cs) {
    struct funcall_mark *fm;

    /*
     * why do we use funcall stack for
     * the currently defined function
     */

    fm = TOP_FUNCALL(cs);
    listAppend(get_func_args(fm->fid),
               cs_get_argname(cs, fm->arg_idx));
    cs_input_id(cs, fm->arg_idx);
}


/* NON-INTERACTIVE */

unsigned int cs_add_const(struct calc_state *cs, double val, char *name)
{
    return add_const(val, cs_add_symbol(cs, name));
}


/* SYMBOL STORAGE */

char *cs_add_symbol(struct calc_state *cs, char *sym) {
    char *heap_sym = alloc_string(sym);
    listPush(cs->symbols, &heap_sym);
    return heap_sym;
}

void cs_remove_symbol(struct calc_state *cs, char *sym) {
    struct list_item *prev = NULL;
    struct list_item *current;
    size_t idx;
    
    FOR_LIST_ITEMS(cs->symbols, idx, current) {
        if (LIST_ITEM_REF(char *, current) == sym) {
            *(prev ? &prev->next : &cs->symbols->first) = current->next;
            free(current);
            break;
        }
        prev = current;
    }

    free(sym);
}

void cs_remove_unneeded(struct calc_state *cs) {
    struct new_name_mark *nnm = TOP_NEW_NAME(cs);

    switch (nnm->type) {
        case ITVar:
            cs_remove_symbol(cs, get_var_name(nnm->id));
            remove_var(nnm->id);
            break;
        case ITFunc:
            cs_remove_symbol(cs, get_func_name(nnm->id));
            remove_func(nnm->id);
            break;
        default:
            /* args use persistent names;
             * constants (incl. named) cannot be inserted,
             * and so cannot be removed
             */
            break;
    }

    listRemove(cs->names, 0);
}


/* MISC */

enum calc_input_type cs_get_input_type(struct calc_state *cs) {
    return cs->cit;
}

char *cs_curr_str(struct calc_state *cs) {
    return cs->str;
}

unsigned int cs_get_func_id(struct calc_state *cs) {
    return cs->scope.fid;
}

struct list_head *cs_get_expr(struct calc_state *cs) {
    return cs->expr;
}

