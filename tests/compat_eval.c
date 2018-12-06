#include "list.h"
#include "eval.h"
#include "compat_eval.h"

// TODO: remove later
struct eval_state *default_eval_state = NULL;


// COMPATIBILITY


void init_calc()
{
    init_eval_state(default_eval_state);
}

void destroy_calc()
{
    destroy_eval_state(default_eval_state);
}

double eval_atom(Token t)
{
    return eval_atom_es(default_eval_state, &t);
}

unsigned int count_var()
{ return default_eval_state->vars->length; }

unsigned int count_const()
{ return default_eval_state->consts->length; }

unsigned int count_func()
{ return default_eval_state->funcs->length; }

static unsigned int add_value(struct list_head *h,
                       double v,
                       char *n)
{
    add_named_value_es(h, v, n, 0);
    return h->length - 1;
}

static double get_value(struct list_head *h, unsigned int id)
{ return LIST_DATA(struct named_value, h, id)->value; }

static char *get_value_name(struct list_head *h, unsigned int id)
{ return LIST_DATA(struct named_value, h, id)->name; }

static void set_value(struct list_head *h,
               unsigned int id,
               double v)
{ LIST_DATA(struct named_value, h, id)->value = v; }

unsigned int add_var(double init_val, char *name)
{ return add_value(default_eval_state->vars, init_val, name); }

double get_var(unsigned int vid)
{ return get_value(default_eval_state->vars, vid); }

char *get_var_name(unsigned int vid)
{ return get_value_name(default_eval_state->vars, vid); }

void set_var(unsigned int vid, double value)
{ set_value(default_eval_state->vars, vid, value); }

void remove_var(unsigned int vid)
{ listRemove(default_eval_state->vars, vid); }

unsigned int add_const(double val, char *name)
{ return add_value(default_eval_state->consts, val, name); }

double get_const(unsigned int cid)
{ return get_value(default_eval_state->consts, cid); }

char *get_const_name(unsigned int cid)
{ return get_value_name(default_eval_state->consts, cid); }

void set_const(unsigned int cid, double val)
{ set_value(default_eval_state->consts, cid, val); }

void remove_const(unsigned int cid)
{ listRemove(default_eval_state->consts, cid); }

unsigned int add_func(char *name)
{
    struct function *f = add_func_es(default_eval_state, name, 0);
    f->args = makeList(sizeof(char *));
    return default_eval_state->funcs->length - 1;
}

// args: list of strings
unsigned int set_func_args(unsigned int fid,
                           struct list_head *args)
{
    struct function *f = LIST_DATA(struct function,
                                   default_eval_state->funcs,
                                   fid);
    if (f->args)
        destroyList(f->args);
    f->args = args;
    return fid;
}

// body: list of tokens
unsigned int set_func_body(unsigned int fid, struct list_head *body)
{
    struct function *f = LIST_DATA(struct function,
                                   default_eval_state->funcs,
                                   fid);

    if (f->defined_p == DEFINED && f->body.tokens)
        destroyList(f->body.tokens);
    f->defined_p = DEFINED;
    f->body.tokens = body;
    return fid;
}

unsigned int set_func_func(unsigned int fid,
                           primitive_function primitive)
{
    struct function *f = LIST_DATA(struct function,
                                   default_eval_state->funcs,
                                   fid);

    if (f->defined_p == DEFINED && f->body.tokens)
        destroyList(f->body.tokens);
    f->defined_p = PRIMITIVE;
    f->body.f = primitive;
    return fid;
}

struct list_head *get_func_args(unsigned int fid)
{
    return LIST_DATA(struct function,
                     default_eval_state->funcs,
                     fid)->args;
}

unsigned int get_func_argc(unsigned int fid)
{
    return get_func_args(fid)->length;
}

char *get_func_arg_name(unsigned int fid, unsigned int arg)
{
    return LIST_REF(char *, get_func_args(fid), arg);
}

char *get_func_name(unsigned int fid)
{
    return LIST_DATA(struct function,
                     default_eval_state->funcs,
                     fid)->name;
}

// args: list of lists of tokens, see call_func()
struct list_head *eval_all_args(struct list_head *args)
{ return eval_all_args_es(default_eval_state, args); }

// args: list of lists of tokens
double call_func(unsigned int fid, struct list_head *args)
{ return call_func_es(default_eval_state, fid, args); }

void remove_func(unsigned int fid)
{ remove_func_es(default_eval_state, fid); }

struct lvalue eval_lvalue(struct list_head *tokens)
{
    struct lvalue x;
    // ignore errors
    eval_lvalue_es(tokens, &x);
    return x;
}

// rvalue: list of tokens
struct lvalue eval_assign(struct lvalue lvalue,
                          struct list_head *rvalue)
{
    eval_assignment_es(default_eval_state, &lvalue, rvalue);
    return lvalue;
}

// ignore op_order
double eval_expr(struct list_head *tokens, int op_order)
{ return eval_expr_es(default_eval_state, tokens); }
