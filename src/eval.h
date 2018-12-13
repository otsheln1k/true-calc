/* -*- mode: c -*- */

#ifndef EVAL_INCLUDED
#define EVAL_INCLUDED

#include <stdbool.h>
#include "list.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


/* TYPES (with COMPATIBILITY TYPEDEFS) */

struct eval_state;

typedef double (*primitive_function)(struct eval_state *e,
                                     struct list_head *args);

enum token_type {
    Var, Const, Func, Number, Operator, UOperator, Arg
};  // Arg is only used in a function body

typedef enum operator_type {
    OAdd, OSub,
    OMul, OTDiv, OFDiv, OMod,
    OPow,
    OLp, ORp,
    OAssign,
    OLCall, OComma, ORCall,
    O_Count
} OperatorType;

struct operator_props {
    int order, nesting;
};

typedef enum uoperator_type {
    UPlus, UMinus
} UOperatorType;

typedef union token_value {
    double number;
    unsigned int id;
    enum operator_type op;
    enum uoperator_type uop;
} TokenValue;

typedef struct token {
    enum token_type type;
    union token_value value;
} Token;

typedef union function_body {
    struct list_head *tokens;  // list of Tokens. may contain ones with .type == Arg
    primitive_function f;  // function pointer. args - list of list of Token
} FunctionBody;

typedef struct function {
    char *name;
    struct list_head *args;     // list of char* - arg names
    enum function_definedness {
        NOT_DEFINED, PRIMITIVE, DEFINED
    } defined_p;
    union function_body body;
} DefinedFunction;  // callable function

typedef struct named_value {
    char *name;
    double value;
} NamedValue;

typedef struct lvalue {
    unsigned int id;
    bool is_func;
} LValue;

struct eval_state {
    struct list_head *vars;
    struct list_head *consts;
    struct list_head *funcs;
};


/* NEW INTERFACE (some symbols will be renamed) */

void init_eval_state(struct eval_state *e);
void destroy_eval_state(struct eval_state *e);

double eval_atom_es(struct eval_state *e, struct token *atom);
struct named_value *add_named_value_es(struct list_head *l,
                                       double init_val,
                                       char *name,
                                       int head_p);
struct function *add_func_es(struct eval_state *e,
                             char *name,
                             int head_p);
void remove_func_es(struct eval_state *e, unsigned int fid);
struct list_head *eval_all_args_es(struct eval_state *e,
                                   struct list_head *args);
struct list_head *eval_arglist_es(struct eval_state *e,
                                  struct list_head *tokens);
double call_func_es(struct eval_state *e,
                    unsigned int fid,
                    struct list_head *argv);
double eval_arithmetic(enum operator_type operation,
                      double op1, double op2);
double eval_uoperation(enum uoperator_type uoperation,
                       double op);
bool eval_lvalue_es(struct list_head *tokens,
                    struct lvalue *ret_lvalue);
const struct operator_props *token_props(struct token *tok);
double eval_assignment_es(struct eval_state *e,
                          struct lvalue *lvalue,
                          struct list_head *rvalue);
struct list_item *find_next_token(struct list_head *tokens,
                                  size_t *out_idx);
double eval_binary_es(struct eval_state *e,
                      enum operator_type op,
                      struct list_head *left,
                      struct list_head *right);
double eval_expr_es(struct eval_state *e,
                    struct list_head *tokens);

#define GET_VAR(e, idx) LIST_DATA(struct named_value, (e)->vars, (idx))
#define GET_CONST(e, idx) LIST_DATA(struct named_value, (e)->consts, (idx))
#define GET_FUNC(e, idx) LIST_DATA(struct function, (e)->funcs, (idx))
#define FUNC_ARG_PTR(f, idx) LIST_DATA(char *, (f)->args, (idx))

#define FUNCDEF(e, fname, fargs, fpredef)               \
    do {                                                \
        struct function *f = add_func_es(e, fname, 0);  \
        f->args = fargs;                                \
        f->defined_p = PRIMITIVE;                       \
        f->body.f = fpredef;                            \
    } while (0);

#define FUNCDEF_ARGS(e, fname, fargc, fargs, fpredef)           \
    FUNCDEF(e, fname, LIST_CONV(char *, fargc, fargs), fpredef)

#define PREDEF_ONELINE_FUNC(name, args_name, argv_name, expr)   \
    static double name(struct eval_state *e,                    \
                       struct list_head *args_name) {           \
        struct list_head *argv_name =                           \
            eval_all_args_es(e, args_name);                     \
        double res = expr;                                      \
        destroyList(argv_name);                                 \
        return res;                                             \
    }


#define GET_PTOKEN(arr,idx) LIST_DATA(Token, arr, idx)
#define GET_DOUBLE(arr, idx) LIST_REF(double, arr, idx)

#endif
