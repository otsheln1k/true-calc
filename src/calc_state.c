#include <stdio.h>
#include <math.h>

#define NEW_FOR_LIST

#include "calc_state.h"
#include "eval.h"
#include "list.h"
#include "ftoa.h"


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
    char *str = malloc(sizeof(char) * len);
    char *c = &str[len - 2];

    strcpy(str, prefix);
    str[len - 1] = 0;
    while (number) {
        *c-- = '0' + (number % 10);
        number /= 10;
    }

    return str;
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
        return GET_VAR(&cs->e, token->value.id)->name;
    case Const:
        return GET_CONST(&cs->e, token->value.id)->name;
    case Func:
        return GET_FUNC(&cs->e, token->value.id)->name;
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
        return *FUNC_ARG_PTR(
            GET_FUNC(
                &cs->e,
                (cs->defun_p == INSIDE_DEFUN)
                ? cs->scope.fid
                : TOP_FUNCALL(cs)->fid),
            token->value.id);
    default:
        return "error";
    }
}


static void cs_reset(struct calc_state *cs) {
    cs->exp = TEValue;
    if (cs->func_args_swapped
        && cs->old_func_args)
        destroyList(cs->old_func_args);
    cs->func_args_swapped = 0;
    cs->nesting = 0;
    cs->str = malloc(CALC_STR_BLOCK_SIZE);
    cs->str[0] = 0;
    cs->str_sz = CALC_STR_BLOCK_SIZE;
    cs->str_len = 1;  // null-terminated string
    cs->no_cb = 0;
    cs->cit = ITNone;
    cs->defun_p = MAY_BE_DEFUN;
}

struct calc_state *cs_create() {
    struct calc_state *cs = malloc(sizeof(*cs));
    init_eval_state(&cs->e);
    cs->expr = makeList(sizeof(struct token));
    cs->callback = NULL;
    cs->eval_cb = NULL;
    cs->final_eval = 0;
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
    listClear(cs->names);
    struct list_head *expr = cs->final_eval
        ? cs->expr : listCopy(cs->expr);
    listReverse(expr);
    double res = eval_expr_es(&cs->e, expr);
    if (!cs->final_eval)
        destroyList(expr);
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
    destroy_eval_state(&cs->e);
    free(cs);
}


void cs_set_cb(struct calc_state *cs,
               calc_state_changed_cb cb,
               calc_eval_cb ecb) {
    cs->callback = cb;
    cs->eval_cb = ecb;
}

/* SYMBOL STORAGE */

static char *cs_get_argname(struct calc_state *cs, unsigned int idx) {
    char *buf;

    for (; cs->argnames <= idx; cs->argnames++) {
        buf = make_prefixed_number_string("p", cs->argnames + 1);
        listInsert(cs->symbols, cs->argnames, &buf);
    }
    return LIST_REF(char *, cs->symbols, idx);
}

char *cs_add_symbol(struct calc_state *cs, char *sym) {
    char *heap_sym = alloc_string(sym);
    listAppend(cs->symbols, &heap_sym);
    return heap_sym;
}

void cs_remove_symbol(struct calc_state *cs, char *sym) {
    struct list_item *prev = NULL;
    struct list_item *current;
    size_t idx;

    FOR_LIST_ITEMS(cs->symbols, idx, current) {
        if (LIST_ITEM_REF(char *, current) == sym) {
            *((prev != NULL) ? &prev->next : &cs->symbols->first)
                = current->next;
            cs->symbols->length -= 1;
            free(current);
            break;
        }
        prev = current;
    }

    free(sym);
}

void cs_remove_unneeded(struct calc_state *cs) {
    struct new_name_mark *nnm;

    if (cs->names->length == 0
        || (nnm = TOP_NEW_NAME(cs))->offset < cs->expr->length - 1)
        return;

    switch (nnm->type) {
        case ITVar:
            cs_remove_symbol(cs, GET_VAR(&cs->e, nnm->id)->name);
            listRemove(cs->e.vars, nnm->id);
            break;
        case ITFunc:
            cs_remove_symbol(cs, GET_FUNC(&cs->e, nnm->id)->name);
            remove_func_es(&cs->e, nnm->id);
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


#define UPDATE_FLAGS(lvalue, addf, remf) { lvalue |= addf; lvalue &= ~(remf); }


enum token_exp _cs_update_expect(struct calc_state *cs,
                                 struct token *new_tok_p)
{
    enum token_exp more = 0;

    if (new_tok_p == NULL)
        return TEValue;

    switch (new_tok_p->type) {
    case Operator:
        switch (new_tok_p->value.op) {
        case ORCall:
            switch (cs->defun_p) {
            case IS_DEFUN:
                return TEAssign;
            case MAY_BE_DEFUN:
                more |= TEAssign;
                // intentionally fall through
            default:
                goto cs_update_expect_binary_op;
            }
            // never reached
        case OLCall:
            switch (cs->defun_p) {
            case IS_DEFUN:
                return TEFRCall | TEFArgs;
            case MAY_BE_DEFUN:
                more |= TEFRCall | TEFArgs;
                break;
            default:
                more |= TEFRCall;
                break;
            }
            break;
        case OComma:
            if (cs->defun_p == IS_DEFUN)
                return TEFArgs;
            break;
       case ORp:
            goto cs_update_expect_binary_op;
         default:
            // expect a value
            break;
        }
        // intentionally fall through
    case UOperator:
        if (cs->defun_p == INSIDE_DEFUN)
            more |= TEFArgs;
        return more | TEValue;
    case Arg:
        if (cs->scope.offset == 0)
            return TEFComma | TEFRCall;
        // else fall through
    case Var:
        if (cs->subexpr == SUBEXPR_LVALUE)
            more |= TEAssign;
        // intentionally fall through
    cs_update_expect_binary_op:
    case Const:
    case Number:
        if (cs->nesting) {
            more |=
                (cs->fcalls->length == 0
                 || cs->nesting - 1 > TOP_FUNCALL(cs)->nesting_level)
                ? TECloseParen
                : (TEFComma | TEFRCall);
        }
        return more | TEBinaryOperator;
    case Func:
        return TEFuncall;
    }
    // never reached
    return TENone;
}

void cs_update_expect(struct calc_state *cs, struct token *new_tok_p) {
    cs->exp = _cs_update_expect(cs, new_tok_p);
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
            return (cs->exp & TEValue) && cs->e.consts->length;
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

static void cs_swap_args(struct calc_state *cs) {
    struct function *f = GET_FUNC(&cs->e, cs->first_fid);
    cs->func_args_swapped = 1;
    cs->old_func_args = f->args;
    f->args = makeList(sizeof(char *));
}

static void cs_revert_swap_args(struct calc_state *cs) {
    struct function *f = GET_FUNC(&cs->e, cs->first_fid);
    destroyList(f->args);
    f->args = cs->old_func_args;
    cs->func_args_swapped = 0;
}

void cs_add_item(struct calc_state *cs, struct token new_tok) {
    enum subexpr_status subexpr = cs->subexpr;
    cs->subexpr = SUBEXPR_NOT_LVALUE;

    if (cs->defun_p == MAY_BE_DEFUN) {
        cs->defun_idx = cs->expr->length;
        switch (cs->expr->length) {
        case 0:
            if (new_tok.type != Func)
                cs->defun_p = IS_NOT_DEFUN;
            break;
        case 1:
            if (new_tok.type != Operator
                || new_tok.value.op != OLCall)
                cs->defun_p = IS_NOT_DEFUN;
            break;
        case 2:
            cs->defun_p =
                (new_tok.type == Arg) ? IS_DEFUN
                : (new_tok.type == Operator
                   && new_tok.value.op == ORCall) ? MAY_BE_DEFUN
                : IS_NOT_DEFUN;
            break;
        case 3:
            cs->defun_p = (new_tok.type == Operator
                           && new_tok.value.op == OAssign)
                ? INSIDE_DEFUN : IS_NOT_DEFUN;
            break;
        default:
            // never reached
            break;
        }
    } else if (cs->defun_p == IS_DEFUN
               && new_tok.type == Operator
               && new_tok.value.op == OAssign) {
        cs->defun_p = INSIDE_DEFUN;
    }

    if (!cs->func_args_swapped
        && (cs->defun_p == IS_DEFUN
            || cs->defun_p == INSIDE_DEFUN)) {
        cs_swap_args(cs);
    } else if (cs->func_args_swapped
               && cs->defun_p == IS_NOT_DEFUN) {
        cs_revert_swap_args(cs);
    }

    if (cs->defun_p == IS_DEFUN
        && new_tok.type == Arg)
        listAppend(GET_FUNC(&cs->e, TOP_FUNCALL(cs)->fid)->args,
                   &cs->new_arg_name);

    if (new_tok.type == Operator) {
        // special actions for special types of Tokens
        switch (new_tok.value.op) {
        case OLCall: {
            struct funcall_mark fm;
            fm.nesting_level = cs->nesting;
            fm.fid = GET_PTOKEN(cs->expr, 0)->value.id;
            fm.arg_idx = 0;
            listPush(cs->fcalls, &fm);

            // set fid here in case of a defun
            if (cs->defun_p == MAY_BE_DEFUN)
                cs->first_fid = fm.fid;
        } // intentionally fall through
        case OLp:
            cs->nesting += 1;
            cs->subexpr = SUBEXPR_BEGIN;
            break;
        case OComma:
            TOP_FUNCALL(cs)->arg_idx += 1;
            cs->subexpr = SUBEXPR_BEGIN;
            break;
        case ORCall:
            listRemove(cs->fcalls, 0);
            // intentionally fall through
        case ORp:
            cs->nesting -= 1;
            break;
        case OAssign:
            if (cs->defun_p == INSIDE_DEFUN
                && cs->scope.offset == 0) {
                // first_fid should be set already
                // defun_p will be set later
                cs->scope.nesting_level = cs->nesting;
                cs->scope.offset = cs->expr->length + 1;
                cs->scope.fid = cs->first_fid;
            }
            break;
        default:
            break;
        }
    } else if (subexpr == SUBEXPR_BEGIN && new_tok.type == Var) {
        cs->subexpr = SUBEXPR_LVALUE;
    }

    listPush(cs->expr, &new_tok);
    cs_update_expect(cs, &new_tok);
    const char *s = cs_get_token_text(cs, &new_tok);
    cs->str_len += strlen(s);
    size_t newbs = CALC_STR_BLOCK_SIZE *
        (1 + (cs->str_len - 1) / CALC_STR_BLOCK_SIZE);
    if (newbs > cs->str_sz) {
        cs->str = realloc(cs->str, newbs);
        cs->str_sz = newbs;
    }
    strcat(cs->str, s);
    cs->cit = ITNone;
    cs_call_cb(cs);
}

static inline int begins_subexpr(struct token *token)
{
    if (token->type != Operator)
        return 0;
    switch (token->value.op) {
    case OLp:
    case OLCall:
    case OComma:
        return 1;
    default:
        return 0;
    }
}

void cs_rem_item(struct calc_state *cs) {
    // note: this is the *new* length
    unsigned int len = cs->expr->length - 1;
    struct token *t = GET_PTOKEN(cs->expr, 0);
    size_t slen = strlen(cs_get_token_text(cs, t));

    if (len == 0
        || begins_subexpr(GET_PTOKEN(cs->expr, 1)))
        cs->subexpr = SUBEXPR_BEGIN;
    else if (len >= 2
             && GET_PTOKEN(cs->expr, 1)->type == Var
             && begins_subexpr(GET_PTOKEN(cs->expr, 2)))
        cs->subexpr = SUBEXPR_LVALUE;
    else
        cs->subexpr = SUBEXPR_NOT_LVALUE;

    switch (cs->defun_p) {
    case INSIDE_DEFUN:
        if (t->type == Operator && t->value.op == OAssign)
            cs->defun_p = IS_DEFUN;
        // intentionally fall through
    case IS_DEFUN:
    case IS_NOT_DEFUN:
        if (cs->defun_idx == len)
            cs->defun_p = MAY_BE_DEFUN;
        break;
    default:
        break;
    }

    switch (t->type) {
    case Operator:
        switch (t->value.op) {
        case OLCall:
            listRemove(cs->fcalls, 0);
            // intentionally fall through
        case OLp:
            cs->nesting -= 1;
            break;
        case OComma:
            TOP_FUNCALL(cs)->arg_idx -= 1;
            break;
        case ORCall: {
            struct funcall_mark fm;
            size_t tindex;
            struct list_item *item;
            struct token *token, *prev = NULL;
            unsigned int nesting = cs->nesting;

            /* find beginning of funcall */

            fm.nesting_level = cs->nesting + 1;
            fm.arg_idx = 0;
            FOR_LIST(cs->expr, tindex, item) {
                token = LIST_ITEM_DATA(struct token, item);

                if (nesting == fm.nesting_level - 1
                    && token->type == Func
                    && prev != NULL
                    && prev->type == Operator
                    && prev->value.op == OLCall) {
                    fm.fid = token->value.id;
                    break;
                } else if (nesting == fm.nesting_level
                           && token->type == Operator
                           && token->value.op == OComma) {
                    fm.arg_idx += 1;
                }

                if (token->type == Operator) {
                    switch (token->value.op) {
                    case ORp:
                    case ORCall:
                        nesting += 1;
                        break;
                    case OLp:
                    case OLCall:
                        nesting -= 1;
                        break;
                    default:
                        break;
                    }
                }

                prev = token;
            }
            listPush(cs->fcalls, &fm);
        } // intentionally fall through
        case ORp:
            cs->nesting += 1;
            break;
        case OAssign:
            cs->subexpr = SUBEXPR_LVALUE;
            break;
        default:
            break;
        }
        break;
    case Arg:
        if (cs->defun_p != INSIDE_DEFUN) {
            struct list_head *fargs =
                GET_FUNC(&cs->e, TOP_FUNCALL(cs)->fid)->args;
            listRemove(fargs, fargs->length - 1);
        }
        break;
    case Var:
    case Const:
    case Func:
        cs_remove_unneeded(cs);
        break;
    default:
        break;
        /*
          && cs->names->length
          && (TOP_NEW_NAME(cs)->offset == len)
        */
        // automaticaly remove the unneeded symbols here
        /* cs_remove_unneeded(cs); */
    /* } */
    }

    if (cs->defun_p == MAY_BE_DEFUN
        && cs->func_args_swapped)
        cs_revert_swap_args(cs);

    cs->str_len -= slen;
    cs->str[cs->str_len - 1] = 0;
    listRemove(cs->expr, 0);
    if (cs->expr->length)
        cs_update_expect(cs, GET_PTOKEN(cs->expr, 0));
    else
        cs->exp = TEValue;
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
        cs_add_const(cs,
                     cs_eval(cs),
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
    cs_add_item(cs, (struct token){
            Number, (TokenValue){ .number = num }});
}

void cs_input_id(struct calc_state *cs, unsigned int id) {
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
    struct new_name_mark nnm;

    switch (cs->cit) {
    case ITVar:
        add_named_value_es(cs->e.vars,
                           0.0,
                           cs_add_symbol(cs, name),
                           0);
        id = cs->e.vars->length - 1;
        break;
    case ITFunc:
        add_func_es(&cs->e,
                    cs_add_symbol(cs, name),
                    0);
        id = cs->e.funcs->length - 1;
        break;
    case ITNC:
        return cs_add_const(cs, cs_eval(cs), name);
    default:
        /* never reached */
        return 0;
    }

    nnm = (struct new_name_mark){cs->expr->length, cs->cit, id};
    listPush(cs->names, &nnm);
    cs_input_id(cs, id);
    return id;
}

void cs_input_newarg(struct calc_state *cs) {
    struct funcall_mark *fm = TOP_FUNCALL(cs);

    cs->new_arg_name = cs_get_argname(cs, fm->arg_idx);
    cs_input_id(cs, fm->arg_idx);
}


/* NON-INTERACTIVE */

unsigned int cs_add_const(struct calc_state *cs,
                          double val,
                          char *name) {
    add_named_value_es(cs->e.consts,
                       val,
                       cs_add_symbol(cs, name),
                       1);
    return 0;
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

struct list_head *cs_get_rev_expr(struct calc_state *cs) {
    static struct list_head *expr = NULL;
    if (expr != NULL)
        destroyList(expr);
    expr = listCopy(cs->expr);
    listReverse(expr);
    return expr;
}
