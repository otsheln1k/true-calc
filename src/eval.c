#include <stdbool.h>
#include <math.h>

#ifdef __PEBBLE__
#include "SmallMaths.h"
#define __pow sm_pow
#else
#define __pow pow
#endif

#include "ieee_fp.h"
#define NEW_FOR_LIST
#include "list.h"
#include "eval.h"

static const struct operator_props operator_props[O_Count] = {
    { 1, 0 }, { 1, 0 },
    { 2, 0 }, { 2, 0 }, { 2, 0 }, { 2, 0 },
    { 3, 0 },
    { -1, 1 }, { -1, -1 },
    { 0, 0 },
    { 5, 1 }, { -1, 0 }, { -1, -1 },
};


void init_eval_state(struct eval_state *e)
{
    e->vars = makeList(sizeof(struct named_value));
    e->consts = makeList(sizeof(struct named_value));
    e->funcs = makeList(sizeof(struct function));
}

void destroy_eval_state(struct eval_state *e)
{
    struct list_item *item;
    size_t idx;
    struct function *func;

    destroyList(e->vars);
    destroyList(e->consts);

    FOR_LIST(e->funcs, idx, item) {
        func = LIST_ITEM_DATA(struct function, item);
        if (func->args)
            destroyList(func->args);
        if (func->defined_p == DEFINED && func->body.tokens)
            destroyList(func->body.tokens);
    }

    destroyList(e->funcs);
}


double eval_atom_es(struct eval_state *e, struct token *atom) {
    switch (atom->type) {
        case Var:
            return LIST_DATA(struct named_value,
                             e->vars,
                             atom->value.id)->value;
        case Const:
            return LIST_DATA(struct named_value,
                             e->consts,
                             atom->value.id)->value;
        case Number:
            return atom->value.number;
        default:
            return -1.0;
    }
}


struct named_value *add_named_value_es(struct list_head *l,
                                       double init_val,
                                       char *name,
                                       int head_p)
{
    struct named_value nv = { .name = name, .value = init_val, };
    return (struct named_value *)
        listInsert(l, head_p ? 0 : l->length, &nv)->data;
}

struct function *add_func_es(struct eval_state *e,
                             char *name,
                             int head_p)
{
    struct function defun = {
        .name = name,
        .args = NULL,
        .defined_p = NOT_DEFINED,
        .body = { .f = NULL },
    };
    return (struct function *)
        listInsert(e->funcs,
                   head_p ? 0 : e->funcs->length,
                   &defun)->data;
}

void remove_func_es(struct eval_state *e, unsigned int fid)
{
    struct list_item *item = listPop(e->funcs, fid);
    struct function *func = LIST_ITEM_DATA(struct function, item);

    if (func->args != NULL)
        destroyList(func->args);
    if (func->defined_p == DEFINED && func->body.tokens != NULL)
        destroyList(func->body.tokens);
    free(item);
}


// EVALUATION

// is it necessary?
struct list_head *eval_all_args_es(struct eval_state *e,
                                   struct list_head *args)
{
    struct list_head *res = makeList(sizeof(double));
    struct list_item *last = NULL;
    struct list_item *item;
    double val;
    size_t idx;
    struct list_item *arg;

    FOR_LIST(args, idx, arg) {
        item = makeListItem(res->elt_size);
        val = eval_expr_es(e, LIST_ITEM_REF(struct list_head *, arg));
        LIST_ITEM_SET(double, item, val);
        if (idx) last = last->next = item;
        else last = res->first = item;
    }

    res->length = args->length;

    return res;
}

struct list_head *eval_arglist_es(struct eval_state *e,
                                  struct list_head *tokens)
{
    struct list_head *hd = makeList(sizeof(struct list_head *));
    struct list_head *part;
    struct list_item *first = tokens->first;
    struct list_item *iter;
    size_t first_idx = 0;
    size_t idx;
    struct token *t;
    int nesting = 0;
    const struct operator_props *props;

    FOR_LIST(tokens, idx, iter) {
        t = LIST_ITEM_DATA(struct token, iter);
        props = token_props(t);
        if (!nesting && t->type == Operator
            && (t->value.op == OComma || t->value.op == ORCall)) {
            if (idx > first_idx) {
                part = takeListItems(first,
                                     idx - first_idx,
                                     tokens->elt_size);
                listInsert(hd, 0, &part);
            }
            if (t->value.op == ORCall)
                break;
            first = iter->next;
            first_idx = idx + 1;
        }
        if (props)
            nesting += props->nesting;
    }

    listReverse(hd);

    return hd;
}

double call_func_es(struct eval_state *e,
                    unsigned int fid,
                    struct list_head *argv)
{
    struct function *func = LIST_DATA(struct function, e->funcs, fid);
    switch (func->defined_p) {
    case NOT_DEFINED:
        return NAN;
    case PRIMITIVE:
        return func->body.f(e, argv);
    default:
        break;
    }

    struct list_head *argval = eval_all_args_es(e, argv);
    struct list_head *body = listCopy(func->body.tokens);
    unsigned int idx;
    struct list_item *item;
    struct token *token;

    FOR_LIST(body, idx, item) {
        token = LIST_ITEM_DATA(struct token, item);
        if (token->type == Arg) {
            token->type = Number;
            token->value.number =
                LIST_REF(double, argval, token->value.id);
        }
    }

    double res = eval_expr_es(e, body);
    destroyList(body);
    destroyList(argval);
    return res;
}

double eval_arithmetic(enum operator_type operation,
                      double op1, double op2)
{
    switch (operation) {
        case OMul:
            return op1 * op2;
        case OTDiv:  // `true´ div, 3/2 -> 1.5
            return op1 / op2;
        case OFDiv:
            return ieee_trunc(op1 / op2);
        case OMod:
            return ieee_fmod(op1, op2);
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

double eval_uoperation(enum uoperator_type uoperation, double op)
{
    switch (uoperation) {
        case UPlus:
            return op;
        case UMinus:
            return -op;
        default:
            return 0;
    }
}

bool eval_lvalue_es(struct list_head *tokens, struct lvalue *ret_lvalue)
{
    bool is_func = false;

    if (tokens->length > 1) {
        struct token *t1 = LIST_DATA(struct token, tokens, 1);
        struct token *t2 = LIST_DATA(struct token, tokens,
                                     tokens->length - 1);
        if (t1->type == Operator && t1->value.op == OLCall
            && t2->type == Operator && t2->value.op == ORCall)
            is_func = true;
        else
            return false;
    }

    ret_lvalue->id = GET_PTOKEN(tokens, 0)->value.id;
    ret_lvalue->is_func = is_func;
    return true;
}

const struct operator_props *token_props(struct token *tok)
{
    static const struct operator_props uop = { 4, 0 };

    switch (tok->type) {
    case Operator:
        return &operator_props[tok->value.op];
    case UOperator:
        return &uop;
    default:
        return NULL;
    }
}

// eval_assign(struct lvalue lvalue, struct list_head *rvalue)
// also another return value

double eval_assignment_es(struct eval_state *e,
                          struct lvalue *lvalue,
                          struct list_head *rvalue)
{
    double val = 0.0;
    if (lvalue->is_func) {
        struct function *f =
            LIST_DATA(struct function, e->funcs, lvalue->id);
        if (f->defined_p == DEFINED && f->body.tokens)
            destroyList(f->body.tokens);
        f->defined_p = DEFINED;
        f->body.tokens = listCopy(rvalue);
    } else {
        val = eval_expr_es(e, rvalue);
        LIST_DATA(struct named_value, e->vars, lvalue->id)->value = val;
    }
    return val;
}

struct list_item *find_next_token(struct list_head *tokens,
                                  size_t *out_idx)
{
    size_t idx;
    struct list_item *item;
    struct token *t;
    const struct operator_props *props;
    int nesting = 0;

    struct list_item *min_item = NULL;
    size_t min_idx;
    int min_order = -1;

    FOR_LIST(tokens, idx, item) {
        t = LIST_ITEM_DATA(struct token, item);
        props = token_props(t);
        if (!props) continue;
        if (!nesting && props->order >= 0 &&
            (min_order < 0 || props->order < min_order)) {
            min_idx = idx;
            min_item = item;
            min_order = props->order;
        }
        nesting += props->nesting;
    }

    if (min_order < 0)
        return NULL;

    if (out_idx)
        *out_idx = min_idx;
    return min_item;
}

double eval_binary_es(struct eval_state *e,
                      enum operator_type op,
                      struct list_head *left,
                      struct list_head *right)
{
    struct list_head *split_args;
    double val;
    struct lvalue lvalue;
    bool ret;

    switch (op) {
    case OLCall:
        split_args = eval_arglist_es(e, right);
        val = call_func_es(e,
                           LIST_ITEM_DATA(
                               struct token,
                               left->first)->value.id,
                           split_args);
        destroyListHeader(split_args);
        return val;
    case OAssign:
        ret = eval_lvalue_es(left, &lvalue);
        // if ret is zero, it’s an error
        return ret ? eval_assignment_es(e, &lvalue, right) : NAN;
    default:
        return eval_arithmetic(op,
                               eval_expr_es(e, left),
                               eval_expr_es(e, right));
    }
}

double eval_expr_es(struct eval_state *e,
                    struct list_head *tokens)
{
    struct token *min_token;
    struct list_item *min_item;
    size_t min_idx;

    // if there is only 1 token, then it’s an atom
    if (tokens->length == 1)
        return eval_atom_es(e, LIST_ITEM_DATA(struct token,
                                              tokens->first));

    // find the topmost operator: zero nesting,
    // lowest precedence (order)
    min_item = find_next_token(tokens, &min_idx);

    // no top-level operator
    if (min_item == NULL) {
        struct list_head *new_list =
            listSlice(tokens, 1, tokens->length - 1);
        double val = eval_expr_es(e, new_list);
        destroyListHeader(new_list);
        return val;
    }

    min_token = LIST_ITEM_DATA(struct token, min_item);

    // unary operators
    if (min_token->type == UOperator) {
        struct list_head *op_expr = listSlice(tokens, min_idx + 1,
                                         tokens->length);
        double operand = eval_expr_es(e, op_expr);
        destroyListHeader(op_expr);
        return eval_uoperation(min_token->value.uop, operand);
    }

    // binary operators
    if (min_token->type == Operator) {
        struct list_head *left = listSlice(tokens, 0, min_idx);
        struct list_head *right =
            takeListItems(min_item->next,
                          tokens->length - min_idx - 1,
                          tokens->elt_size);

        double val = eval_binary_es(e, min_token->value.op, left, right);

        destroyListHeader(left);
        destroyListHeader(right);
        return val;
    }

    // instead of error
    return NAN;
}
